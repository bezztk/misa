#include "LearningView.h"

#include "CommentUi.h"

#include <QColor>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPalette>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

namespace {

constexpr int contentWidth = 960;
constexpr int pageMargin = 24;
constexpr int actionHeight = 60;
constexpr int ratingWidth = 170;
constexpr int controlSpacing = 12;

struct RatingOption {
    const char *title;
    const char *accent;
    FlashcardRating rating;
    float FlashcardReviewPreview::*interval;
};

constexpr std::array<RatingOption, 4> ratingOptions{{
    {"Nochmal", "#C45D5D", FlashcardRating::Again, &FlashcardReviewPreview::again},
    {"Schwer", "#C49550", FlashcardRating::Hard, &FlashcardReviewPreview::hard},
    {"Gut", "#65A875", FlashcardRating::Good, &FlashcardReviewPreview::good},
    {"Einfach", "#6495C4", FlashcardRating::Easy, &FlashcardReviewPreview::easy},
}};

constexpr std::array<const char *, 5> metadataCaptions{
    "Zustand", "Retention", "Stabilität", "Schwierigkeit", "Lernschritt"
};

constexpr const char *learningStyles = R"(
    QLabel#learningTitle { font-size: 26px; font-weight: 600; }
    QLabel#learningDueCount { font-size: 54px; font-weight: 600; }
    QLabel#learningMutedText { color: %1; }
    QLabel#learningQuestion { font-size: 24px; font-weight: 600; }
    QLabel#learningAnswer { font-size: 18px; }
    QScrollArea#learningCardScroll { background: transparent; }
    QFrame#learningAnswerDivider { background: rgba(127, 127, 127, 60); }
    QProgressBar#learningProgress {
        background: rgba(127, 127, 127, 50);
        border: none;
        border-radius: 2px;
    }
    QProgressBar#learningProgress::chunk {
        background: rgba(127, 127, 127, 160);
        border-radius: 2px;
    }
    QToolButton#learningInfo::menu-indicator { image: none; width: 0; }
    QLabel#learningRatingTitle { font-size: 15px; font-weight: 600; }
    QLabel#learningRatingInterval { font-size: 12px; }
)";

QString ratingStyle(const QColor &accent) {
    const auto tint = [&accent](int alpha) {
        return QString("rgba(%1, %2, %3, %4)")
            .arg(accent.red()).arg(accent.green()).arg(accent.blue()).arg(alpha);
    };
    return QString(R"(
        QPushButton#learningRating {
            border: 1px solid %1;
            border-radius: 6px;
            background: %2;
            padding: 0;
        }
        QPushButton#learningRating:hover { background: %3; }
        QPushButton#learningRating:pressed { background: %4; }
        QPushButton#learningRating:focus { border-color: palette(highlight); }
        QPushButton#learningRating:disabled {
            border-color: %5;
            background: %6;
        }
    )").arg(tint(150), tint(27), tint(51), tint(76), tint(65), tint(14));
}

QString formatInterval(const float days) {
    const double minutes = days * 24 * 60;

    if (minutes < 60) {
        return QString::number(qRound(minutes)) + " Min.";
    }

    if (minutes < 1440) {
        return QString::number(qRound(minutes / 60)) + " Std.";
    }

    return QString::number(qRound(days)) + " Tage";
}

QString flashcardStateName(const int state) {
    switch (state) {
        case 1: return "Neu";
        case 2: return "Lernen";
        case 3: return "Wiederholen";
        case 4: return "Erneut lernen";
        default: return "Unbekannt";
    }
}

QString retentionLevelText(const Metadata &metadata, const int id) {
    for (const FlashcardRetentionClass &level : metadata.flashcardRetentionClasses) {
        if (level.id == id) {
            return QString("%1 (%2 %)")
                .arg(level.displayName.value_or(level.name))
                .arg(qRound(level.desiredRetention * 100));
        }
    }

    return "#" + QString::number(id);
}

QString metricText(const std::optional<float> value) {
    return value ? QString::number(*value, 'f', 2) : "–";
}

} // namespace

LearningView::LearningView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
    const bool darkBackground = palette().color(QPalette::Window).lightness() < 128;
    setStyleSheet(QString(learningStyles).arg(darkBackground ? "#a7a7a7" : "#626262"));

    auto *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    screens = new QStackedWidget(this);
    screens->setMaximumWidth(contentWidth);
    outerLayout->addStretch();
    outerLayout->addWidget(screens, 1);
    outerLayout->addStretch();

    startScreen = createStartScreen();
    sessionScreen = createSessionScreen();
    screens->addWidget(startScreen);
    screens->addWidget(sessionScreen);

    connect(&apiClient, &ApiClient::flashcardsReceived, this,
            [this](const std::vector<FlashcardReviewQueueItem> &receivedFlashcards) {
                flashcards = receivedFlashcards;
                currentIndex = 0;
                showStartScreen();
            });
    connect(&apiClient, &ApiClient::flashcardRated, this, [this] {
        ratingPending = false;
        ratingWidget->setEnabled(true);
        ++currentIndex;
        showCurrentFlashcard();
    });
    connect(&apiClient, &ApiClient::flashcardRatingFailed, this, [this] {
        ratingPending = false;
        ratingWidget->setEnabled(true);
    });

    reloadFlashcards();
}

QWidget *LearningView::createStartScreen() {
    auto *screen = new QWidget(screens);
    auto *layout = new QVBoxLayout(screen);
    layout->setContentsMargins(pageMargin, pageMargin, pageMargin, pageMargin);
    layout->setSpacing(8);

    auto *title = new QLabel("Lernkarten", screen);
    title->setObjectName("learningTitle");
    dueCountLabel = new QLabel(screen);
    dueCountLabel->setObjectName("learningDueCount");
    dueCaption = new QLabel("Fällige Karten", screen);
    dueCaption->setObjectName("learningMutedText");
    dueBreakdown = new QLabel(screen);
    dueBreakdown->setObjectName("learningMutedText");
    dueBreakdown->setWordWrap(true);
    emptyNotice = new QLabel("Aktuell sind keine Lernkarten fällig.", screen);
    emptyNotice->setObjectName("learningMutedText");
    emptyNotice->setWordWrap(true);
    startButton = new QPushButton("Lernen starten", screen);
    startButton->setMinimumSize(160, 42);

    layout->addStretch();
    for (auto *label : {title, dueCountLabel, dueCaption, dueBreakdown, emptyNotice}) {
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);
    }
    layout->addWidget(startButton, 0, Qt::AlignHCenter);
    layout->addStretch();

    connect(startButton, &QPushButton::clicked, this, &LearningView::showCurrentFlashcard);
    return screen;
}

QWidget *LearningView::createSessionScreen() {
    auto *screen = new QWidget(screens);
    auto *layout = new QVBoxLayout(screen);
    layout->setContentsMargins(pageMargin, pageMargin, pageMargin, pageMargin);
    layout->setSpacing(20);

    auto *header = new QGridLayout();
    header->setColumnStretch(0, 1);
    header->setColumnStretch(2, 1);
    header->setHorizontalSpacing(controlSpacing);

    remainingLabel = new QLabel(screen);
    remainingLabel->setObjectName("learningMutedText");
    header->addWidget(remainingLabel, 0, 0);

    progress = new QProgressBar(screen);
    progress->setObjectName("learningProgress");
    progress->setAccessibleName("Sitzungsfortschritt");
    progress->setTextVisible(false);
    progress->setFixedHeight(4);
    progress->setMinimumWidth(36);
    progress->setMaximumWidth(120);
    header->addWidget(progress, 0, 1, Qt::AlignVCenter);

    auto *headerActions = new QHBoxLayout();
    headerActions->addStretch();
    commentButton = new QPushButton(screen);
    commentButton->setIcon(QIcon(":/assets/icons/speech-bubble.png"));
    commentButton->setToolTip("Kommentare anzeigen");
    commentButton->setAccessibleName("Kommentare anzeigen");
    headerActions->addWidget(commentButton);
    connect(commentButton, &QPushButton::clicked, this, &LearningView::showCurrentComments);

    auto *infoButton = new QToolButton(screen);
    infoButton->setObjectName("learningInfo");
    infoButton->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    infoButton->setToolTip("Karteninformationen");
    infoButton->setAccessibleName("Karteninformationen");
    infoButton->setPopupMode(QToolButton::InstantPopup);
    metadataMenu = new QMenu(infoButton);
    auto *metadataAction = new QWidgetAction(metadataMenu);
    auto *metadata = new QWidget(metadataMenu);
    auto *metadataLayout = new QGridLayout(metadata);
    metadataLayout->setContentsMargins(14, 10, 14, 10);
    metadataLayout->setHorizontalSpacing(20);
    for (std::size_t i = 0; i < metadataCaptions.size(); ++i) {
        auto *caption = new QLabel(metadataCaptions[i], metadata);
        caption->setObjectName("learningMutedText");
        metadataValues[i] = new QLabel(metadata);
        metadataValues[i]->setTextFormat(Qt::PlainText);
        const int row = static_cast<int>(i);
        metadataLayout->addWidget(caption, row, 0);
        metadataLayout->addWidget(metadataValues[i], row, 1);
    }
    metadataAction->setDefaultWidget(metadata);
    metadataMenu->addAction(metadataAction);
    infoButton->setMenu(metadataMenu);
    headerActions->addWidget(infoButton);

    auto *endButton = new QPushButton("Lernen beenden", screen);
    headerActions->addWidget(endButton);
    connect(endButton, &QPushButton::clicked, this, &LearningView::showStartScreen);
    header->addLayout(headerActions, 0, 2);
    layout->addLayout(header);

    cardScrollArea = new QScrollArea(screen);
    cardScrollArea->setObjectName("learningCardScroll");
    cardScrollArea->setFrameShape(QFrame::NoFrame);
    cardScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    cardScrollArea->setWidgetResizable(true);

    learningCard = new QWidget();
    auto *cardLayout = new QVBoxLayout(learningCard);
    cardLayout->setContentsMargins(0, 8, 8, 0);
    cardLayout->setSpacing(16);

    front = new QLabel(learningCard);
    front->setObjectName("learningQuestion");
    front->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    front->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    front->setWordWrap(true);
    cardLayout->addWidget(front);

    auto *divider = new QFrame(learningCard);
    divider->setObjectName("learningAnswerDivider");
    divider->setFixedHeight(1);
    answerDivider = divider;
    cardLayout->addWidget(answerDivider);

    back = new QLabel(learningCard);
    back->setObjectName("learningAnswer");
    back->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    back->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    back->setWordWrap(true);
    cardLayout->addWidget(back);
    cardLayout->addStretch();

    cardScrollArea->setWidget(learningCard);
    learningCard->setAutoFillBackground(false);
    cardScrollArea->viewport()->setAutoFillBackground(false);
    layout->addWidget(cardScrollArea, 1);

    learningCard->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(learningCard, &QWidget::customContextMenuRequested, this,
            [this](const QPoint &position) {
                if (currentIndex >= flashcards.size()) {
                    return;
                }
                QMenu menu(learningCard);
                QAction *commentsAction = menu.addAction(
                    QIcon(":/assets/icons/speech-bubble.png"),
                    "Kommentare anzeigen"
                );
                if (menu.exec(learningCard->mapToGlobal(position)) == commentsAction) {
                    showCurrentComments();
                }
            });

    actions = new QStackedWidget(screen);
    actions->setFixedHeight(actionHeight);
    actions->setMaximumWidth(ratingWidth * 4 + controlSpacing * 3);

    revealWidget = new QWidget(actions);
    auto *revealLayout = new QHBoxLayout(revealWidget);
    revealLayout->setContentsMargins(0, 0, 0, 0);
    auto *showAnswerButton = new QPushButton("Antwort anzeigen", revealWidget);
    showAnswerButton->setFixedSize(184, actionHeight);
    revealLayout->addWidget(showAnswerButton, 0, Qt::AlignHCenter);
    connect(showAnswerButton, &QPushButton::clicked, this, &LearningView::showAnswer);
    actions->addWidget(revealWidget);

    ratingWidget = new QWidget(actions);
    auto *ratingLayout = new QHBoxLayout(ratingWidget);
    ratingLayout->setContentsMargins(0, 0, 0, 0);
    ratingLayout->setSpacing(controlSpacing);
    for (std::size_t i = 0; i < ratingOptions.size(); ++i) {
        const auto &option = ratingOptions[i];
        auto *button = new QPushButton(ratingWidget);
        button->setObjectName("learningRating");
        button->setStyleSheet(ratingStyle(QColor(option.accent)));
        button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        button->setFixedHeight(actionHeight);
        button->setMaximumWidth(ratingWidth);

        auto *labels = new QVBoxLayout(button);
        labels->setContentsMargins(0, 0, 0, 0);
        labels->setSpacing(2);
        auto *title = new QLabel(option.title, button);
        title->setObjectName("learningRatingTitle");
        title->setAttribute(Qt::WA_TransparentForMouseEvents);
        auto *interval = new QLabel(button);
        interval->setObjectName("learningRatingInterval");
        interval->setAttribute(Qt::WA_TransparentForMouseEvents);
        labels->addStretch();
        labels->addWidget(title, 0, Qt::AlignHCenter);
        labels->addWidget(interval, 0, Qt::AlignHCenter);
        labels->addStretch();

        ratingControls[i] = {button, interval};
        ratingLayout->addWidget(button, 1);
        connect(button, &QPushButton::clicked, this, [this, rating = option.rating] {
            rateFlashcard(rating);
        });
    }
    actions->addWidget(ratingWidget);

    auto *footer = new QHBoxLayout();
    footer->addStretch();
    footer->addWidget(actions, 1);
    footer->addStretch();
    layout->addLayout(footer);

    return screen;
}

void LearningView::reloadFlashcards() {
    if (ratingPending) {
        return;
    }

    if (flashcards.empty()) {
        showStartScreen();
    }
    apiClient.getDueFlashcards();
}

void LearningView::showStartScreen() {
    const bool hasDueCards = currentIndex < flashcards.size();
    const auto dueCount = hasDueCards ? flashcards.size() - currentIndex : 0;
    std::size_t newCount = 0;
    std::size_t learningCount = 0;
    std::size_t reviewCount = 0;
    for (std::size_t i = currentIndex; i < flashcards.size(); ++i) {
        switch (flashcards[i].reviewStateId) {
            case 1: ++newCount; break;
            case 2:
            case 4: ++learningCount; break;
            case 3: ++reviewCount; break;
        }
    }

    dueCountLabel->setText(QString::number(dueCount));
    dueBreakdown->setText(QString("Neu: %1  ·  Im Lernen: %2  ·  Wiederholungen: %3")
        .arg(newCount).arg(learningCount).arg(reviewCount));
    dueCountLabel->setVisible(hasDueCards);
    dueCaption->setVisible(hasDueCards);
    dueBreakdown->setVisible(hasDueCards);
    startButton->setVisible(hasDueCards);
    emptyNotice->setVisible(!hasDueCards);

    metadataMenu->hide();
    screens->setCurrentWidget(startScreen);
}

void LearningView::rateFlashcard(FlashcardRating rating) {
    if (ratingPending || currentIndex >= flashcards.size()) {
        return;
    }

    ratingPending = true;
    ratingWidget->setEnabled(false);
    const auto ms = static_cast<int>(flashcardShownAt.msecsTo(answerOpenedAt));

    apiClient.rateFlashcard(
        flashcards[currentIndex].id,
        ReviewFlashcard{
            .rating = static_cast<std::int16_t>(rating),
            .durationMs = ms
        });
}

void LearningView::showCurrentFlashcard() {
    if (currentIndex >= flashcards.size()) {
        showStartScreen();
        return;
    }

    flashcardShownAt = QDateTime::currentDateTime();
    const FlashcardReviewQueueItem &flashcard = flashcards[currentIndex];
    front->setText(flashcard.front);
    answerDivider->hide();
    back->hide();
    back->clear();
    metadataMenu->hide();
    updateMetadata();
    updateCommentButton();

    const auto remaining = flashcards.size() - currentIndex;
    remainingLabel->setText(remaining == 1 ? "1 Karte verbleibend"
                                          : QString("%1 Karten verbleibend").arg(remaining));
    progress->setRange(0, static_cast<int>(flashcards.size()));
    progress->setValue(static_cast<int>(currentIndex));

    for (std::size_t i = 0; i < ratingOptions.size(); ++i) {
        const auto &option = ratingOptions[i];
        const QString interval = formatInterval(flashcard.preview.*option.interval);
        ratingControls[i].interval->setText(interval);
        ratingControls[i].button->setAccessibleName(QString("%1, %2").arg(option.title, interval));
    }

    actions->setCurrentWidget(revealWidget);
    cardScrollArea->verticalScrollBar()->setValue(0);
    screens->setCurrentWidget(sessionScreen);
}

void LearningView::showAnswer() {
    answerOpenedAt = QDateTime::currentDateTime();
    back->setText(flashcards[currentIndex].back);
    answerDivider->show();
    back->show();
    actions->setCurrentWidget(ratingWidget);
}

void LearningView::hideEvent(QHideEvent* event) {
    showStartScreen();
    QWidget::hideEvent(event);
}

void LearningView::refreshMetadata() {
    if (screens->currentWidget() == sessionScreen && currentIndex < flashcards.size()) {
        updateMetadata();
    }
}

void LearningView::updateMetadata() {
    const FlashcardReviewQueueItem &flashcard = flashcards[currentIndex];
    metadataValues[0]->setText(flashcardStateName(flashcard.reviewStateId));
    metadataValues[1]->setText(retentionLevelText(apiClient.metadata(), flashcard.retentionClassId));
    metadataValues[2]->setText(metricText(flashcard.stability));
    metadataValues[3]->setText(metricText(flashcard.difficulty));
    metadataValues[4]->setText(QString::number(flashcard.learningStep));
}

void LearningView::showCurrentComments() {
    if (currentIndex >= flashcards.size()) {
        return;
    }
    FlashcardReviewQueueItem &flashcard = flashcards[currentIndex];
    showCommentsDialog(
        learningCard,
        apiClient,
        flashcard.id,
        "Kommentare – Lernkarte",
        flashcard.comments
    );
    updateCommentButton();
}

void LearningView::updateCommentButton() {
    if (currentIndex >= flashcards.size() || flashcards[currentIndex].comments.empty()) {
        commentButton->hide();
        return;
    }
    commentButton->setText(
        QString::number(static_cast<int>(flashcards[currentIndex].comments.size()))
    );
    commentButton->show();
}
