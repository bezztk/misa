#include "LearningView.h"

#include "CommentUi.h"

#include <QColor>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QPalette>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QStackedWidget>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

namespace {

constexpr int cardWidth = 760;
constexpr int pageMargin = 24;
constexpr int actionHeight = 56;
constexpr int ratingWidth = 170;
constexpr int controlSpacing = 12;

struct RatingOption {
    const char *title;
    FlashcardRating rating;
    float FlashcardReviewPreview::*interval;
};

constexpr std::array<RatingOption, 4> ratingOptions{{
    {"Nochmal", FlashcardRating::Again, &FlashcardReviewPreview::again},
    {"Schwer", FlashcardRating::Hard, &FlashcardReviewPreview::hard},
    {"Gut", FlashcardRating::Good, &FlashcardReviewPreview::good},
    {"Einfach", FlashcardRating::Easy, &FlashcardReviewPreview::easy},
}};

constexpr std::array<const char *, 5> metadataCaptions{
    "Zustand", "Retention", "Stabilität", "Schwierigkeit", "Lernschritt"
};

constexpr const char *learningStyles = R"(
    QLabel#learningDueCount { font-size: 54px; font-weight: 600; }
    QLabel#learningDueBreakdown { font-size: 15px; color: %1; }
    QLabel#learningMutedText { color: %1; }
    QPushButton#learningStart { font-size: 16px; }
    QLabel#learningState {
        color: %1;
        background: rgba(127, 127, 127, 32);
        padding: 3px 6px;
    }
    QLabel#learningQuestion { font-size: 34px; font-weight: 600; }
    QLabel#learningHint { font-size: 18px; color: %1; }
    QLabel#learningAnswer { font-size: 26px; }
    QScrollArea#learningCardScroll { background: transparent; }
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
)";

void retainHiddenSpace(QWidget &widget) {
    auto policy = widget.sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    widget.setSizePolicy(policy);
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

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    screens = new QStackedWidget(this);
    outerLayout->addWidget(screens);

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
        ++currentIndex;
        if (screens->currentWidget() == sessionScreen) {
            showCurrentFlashcard();
        } else {
            showStartScreen();
        }
    });
    connect(&apiClient, &ApiClient::flashcardRatingFailed, this, [this] {
        ratingPending = false;
        updateSessionActions();
    });

    reloadFlashcards();
}

QWidget *LearningView::createStartScreen() {
    auto *screen = new QWidget(screens);
    auto *layout = new QVBoxLayout(screen);
    layout->setContentsMargins(pageMargin, pageMargin, pageMargin, pageMargin);
    layout->setSpacing(20);

    auto *summary = new QWidget(screen);
    auto *summaryLayout = new QHBoxLayout(summary);
    summaryLayout->setContentsMargins(0, 0, 0, 0);
    summaryLayout->setSpacing(24);
    auto *total = new QVBoxLayout;
    total->setSpacing(0);
    dueCountLabel = new QLabel(summary);
    dueCountLabel->setObjectName("learningDueCount");
    dueCountLabel->setAlignment(Qt::AlignCenter);
    auto *dueCaption = new QLabel("Fällige Karten", summary);
    dueCaption->setObjectName("learningMutedText");
    dueCaption->setAlignment(Qt::AlignCenter);
    total->addWidget(dueCountLabel);
    total->addWidget(dueCaption);
    summaryLayout->addLayout(total);
    dueBreakdown = new QLabel(summary);
    dueBreakdown->setObjectName("learningDueBreakdown");
    summaryLayout->addWidget(dueBreakdown, 0, Qt::AlignVCenter);

    startButton = new QPushButton("Lernen starten", screen);
    startButton->setObjectName("learningStart");
    startButton->setMinimumSize(184, 48);

    layout->addStretch();
    layout->addWidget(summary, 0, Qt::AlignHCenter);
    layout->addWidget(startButton, 0, Qt::AlignHCenter);
    layout->addStretch();

    connect(startButton, &QPushButton::clicked, this, &LearningView::showCurrentFlashcard);
    return screen;
}

QWidget *LearningView::createSessionScreen() {
    auto *screen = new QWidget(screens);
    screen->setFocusPolicy(Qt::ClickFocus);
    auto *layout = new QVBoxLayout(screen);
    layout->setContentsMargins(pageMargin, pageMargin, pageMargin, pageMargin);
    layout->setSpacing(20);

    auto *header = new QGridLayout();
    header->setColumnStretch(0, 1);
    header->setColumnStretch(2, 1);
    header->setHorizontalSpacing(controlSpacing);

    auto *status = new QHBoxLayout;
    remainingLabel = new QLabel(screen);
    stateLabel = new QLabel(screen);
    stateLabel->setObjectName("learningState");
    status->addWidget(remainingLabel);
    status->addWidget(stateLabel);
    status->addStretch();
    header->addLayout(status, 0, 0);

    progress = new QProgressBar(screen);
    progress->setObjectName("learningProgress");
    progress->setAccessibleName("Sitzungsfortschritt");
    progress->setTextVisible(false);
    progress->setFixedHeight(4);
    progress->setMinimumWidth(48);
    progress->setMaximumWidth(144);
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
    metadataMenu->setObjectName("learningMetadataPopup");
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

    auto *endButton = new QPushButton("Lernsession beenden", screen);
    headerActions->addWidget(endButton);
    connect(endButton, &QPushButton::clicked, this, &LearningView::showStartScreen);
    header->addLayout(headerActions, 0, 2);
    layout->addLayout(header);

    cardScrollArea = new QScrollArea(screen);
    cardScrollArea->setObjectName("learningCardScroll");
    cardScrollArea->setFrameShape(QFrame::NoFrame);
    cardScrollArea->setWidgetResizable(true);
    cardScrollArea->setMaximumWidth(cardWidth);

    learningCard = new QWidget();
    auto *cardLayout = new QVBoxLayout(learningCard);
    cardLayout->setContentsMargins(12, 8, 12, 8);
    auto *fields = new QGridLayout;
    fields->setVerticalSpacing(16);
    fields->setRowMinimumHeight(0, 88);
    fields->setRowMinimumHeight(1, 76);
    fields->setRowMinimumHeight(2, 112);

    front = new QLabel(learningCard);
    front->setObjectName("learningQuestion");
    hint = new QLabel(learningCard);
    hint->setObjectName("learningHint");
    hint->setTextFormat(Qt::PlainText);
    back = new QLabel(learningCard);
    back->setObjectName("learningAnswer");
    for (auto *label : {front, hint, back}) {
        label->setAlignment(Qt::AlignCenter);
        label->setWordWrap(true);
    }

    revealHint = new QPushButton("Hinweis anzeigen (I)", learningCard);
    revealAnswer = new QPushButton("Antwort anzeigen (Leertaste)", learningCard);
    for (auto *button : {revealHint, revealAnswer}) {
        button->setMinimumSize(184, 48);
        retainHiddenSpace(*button);
    }
    retainHiddenSpace(*hint);
    retainHiddenSpace(*back);
    fields->addWidget(front, 0, 0);
    fields->addWidget(hint, 1, 0);
    fields->addWidget(revealHint, 1, 0, Qt::AlignCenter);
    fields->addWidget(back, 2, 0);
    fields->addWidget(revealAnswer, 2, 0, Qt::AlignCenter);
    cardLayout->addStretch(1);
    cardLayout->addLayout(fields);
    cardLayout->addStretch(2);

    cardScrollArea->setWidget(learningCard);
    learningCard->setAutoFillBackground(false);
    cardScrollArea->viewport()->setAutoFillBackground(false);
    auto *cardBand = new QHBoxLayout;
    cardBand->addStretch();
    cardBand->addWidget(cardScrollArea, 24);
    cardBand->addStretch();
    layout->addLayout(cardBand, 1);

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

    const auto makeShortcut = [screen](const Qt::Key key, QPushButton *button) {
        auto *shortcut = new QShortcut(QKeySequence(key), screen);
        shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        shortcut->setAutoRepeat(false);
        shortcut->setEnabled(false);
        connect(shortcut, &QShortcut::activated, button, &QPushButton::click);
        return shortcut;
    };
    hintShortcut = makeShortcut(Qt::Key_I, revealHint);
    answerShortcut = makeShortcut(Qt::Key_Space, revealAnswer);
    connect(revealHint, &QPushButton::clicked, this, &LearningView::showHint);
    connect(revealAnswer, &QPushButton::clicked, this, &LearningView::showAnswer);

    ratingWidget = new QWidget(screen);
    auto *ratingLayout = new QHBoxLayout(ratingWidget);
    ratingLayout->setContentsMargins(0, 0, 0, 0);
    ratingLayout->setSpacing(controlSpacing);
    ratingLayout->addStretch();
    constexpr std::array keys{Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4};
    for (std::size_t i = 0; i < ratingOptions.size(); ++i) {
        auto *button = new QPushButton(ratingWidget);
        button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        button->setFixedHeight(actionHeight);
        button->setMaximumWidth(ratingWidth);
        ratingLayout->addWidget(button, 8);
        ratingControls[i] = {button, makeShortcut(keys[i], button)};
        connect(button, &QPushButton::clicked, this, [this, rating = ratingOptions[i].rating] {
            rateFlashcard(rating);
        });
    }
    ratingLayout->addStretch();
    ratingWidget->setFixedHeight(actionHeight);
    retainHiddenSpace(*ratingWidget);
    ratingWidget->hide();
    layout->addWidget(ratingWidget);

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
    dueBreakdown->setText(QString("%1 Neu\n%2 Im Lernen\n%3 Wiederholungen")
        .arg(newCount).arg(learningCount).arg(reviewCount));
    startButton->setVisible(hasDueCards);

    metadataMenu->hide();
    screens->setCurrentWidget(startScreen);
    updateSessionActions();
}

void LearningView::rateFlashcard(FlashcardRating rating) {
    if (!ratingWidget->isEnabled() || currentIndex >= flashcards.size()) {
        return;
    }

    sessionScreen->setFocus(Qt::OtherFocusReason);
    ratingPending = true;
    updateSessionActions();
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

    const bool enteringSession = screens->currentWidget() != sessionScreen;
    flashcardShownAt = QDateTime::currentDateTime();
    const FlashcardReviewQueueItem &flashcard = flashcards[currentIndex];
    front->setText(flashcard.front);
    hint->setText(flashcard.hint);
    back->setText(flashcard.back);
    hint->hide();
    back->hide();
    revealHint->setVisible(!flashcard.hint.isEmpty());
    revealAnswer->show();
    ratingWidget->hide();
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
        ratingControls[i].button->setText(QString("%1 (%2)\n%3").arg(option.title).arg(i + 1).arg(interval));
    }

    screens->setCurrentWidget(sessionScreen);
    updateSessionActions();
    cardScrollArea->verticalScrollBar()->setValue(0);
    if (enteringSession) {
        sessionScreen->setFocus(Qt::OtherFocusReason);
    }
}

void LearningView::showHint() {
    revealHint->hide();
    hint->show();
    updateSessionActions();
    sessionScreen->setFocus(Qt::OtherFocusReason);
}

void LearningView::showAnswer() {
    answerOpenedAt = QDateTime::currentDateTime();
    revealAnswer->hide();
    back->show();
    ratingWidget->show();
    updateSessionActions();
    sessionScreen->setFocus(Qt::OtherFocusReason);
}

void LearningView::updateSessionActions() {
    const bool active = screens->currentWidget() == sessionScreen;
    hintShortcut->setEnabled(active && !revealHint->isHidden());
    answerShortcut->setEnabled(active && !revealAnswer->isHidden());
    const bool canRate = active && !ratingPending && !ratingWidget->isHidden();
    ratingWidget->setEnabled(canRate);
    for (const auto &control : ratingControls) {
        control.shortcut->setEnabled(canRate);
    }
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
    const QString state = flashcardStateName(flashcard.reviewStateId);
    stateLabel->setText(state);
    metadataValues[0]->setText(state);
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
