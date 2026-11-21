#include "LearningView.h"

#include "CommentUi.h"

#include <QHideEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

static QString formatInterval(const float days) {
    const double minutes = days * 24 * 60;

    if (minutes < 60) {
        return QString::number(qRound(minutes)) + " Min.";
    }

    if (minutes < 1440) {
        return QString::number(qRound(minutes / 60)) + " Std.";
    }

    return QString::number(qRound(days)) + " Tage";
}

static QString flashcardStateName(const int state) {
    switch (state) {
        case 1: return "Neu";
        case 2: return "Lernen";
        case 3: return "Wiederholen";
        case 4: return "Erneut lernen";
        default: return "Unbekannt";
    }
}

static QString retentionLevelText(const Metadata &metadata, const int id) {
    for (const FlashcardRetentionClass &level : metadata.flashcardRetentionClasses) {
        if (level.id == id) {
            return QString("%1 (%2 %)")
                .arg(level.displayName.value_or(level.name))
                .arg(qRound(level.desiredRetention * 100));
        }
    }

    return "#" + QString::number(id);
}

static QString metricText(const std::optional<float> value) {
    return value ? QString::number(*value, 'f', 2) : "–";
}

LearningView::LearningView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
    auto *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto *content = new QWidget(this);
    content->setMaximumWidth(860);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto* headerLayout = new QHBoxLayout();
    pauseButton = new QPushButton("Pause", content);
    pauseButton->hide();

    remainingLabel = new QLabel(content);
    remainingLabel->setStyleSheet(R"(
        color: #969696;
        font-size: 11px;
    )");
    headerLayout->addWidget(remainingLabel, 1);
    headerLayout->addStretch();
    headerLayout->addWidget(pauseButton);
    layout->addLayout(headerLayout);

    learningCard = new QFrame(content);
    auto *card = learningCard;
    card->setObjectName("learningCard");
    card->setStyleSheet(R"(
        QFrame#learningCard {
            background: rgba(255, 255, 255, 0.04);
            border: 1px solid rgba(255, 255, 255, 0.10);
            border-radius: 0;
        }
        QFrame#learningCard QLabel {
            background: transparent;
            border: none;
        }
    )");
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    cardLayout->setSpacing(10);

    front = new QLabel(card);
    front->setText("Lernkarten werden geladen");
    front->setWordWrap(true);
    QFont frontFont = front->font();
    frontFont.setPointSize(frontFont.pointSize() + 2);
    frontFont.setBold(true);
    front->setFont(frontFont);
    backLabel = new QLabel("Antwort", card);
    backLabel->setStyleSheet("color: #969696; font-size: 11px;");
    backLabel->hide();
    back = new QLabel(card);
    back->setWordWrap(true);

    showAnswerButton = new QPushButton("Antwort anzeigen", card);
    showAnswerButton->setMinimumWidth(160);
    showAnswerButton->setMaximumWidth(200);
    showAnswerButton->hide();
    startButton = new QPushButton("Starten", card);
    startButton->setMinimumWidth(120);
    startButton->setMaximumWidth(160);
    startButton->hide();

    cardLayout->addWidget(front);
    cardLayout->addSpacing(12);
    cardLayout->addWidget(backLabel);
    cardLayout->addWidget(back);
    cardLayout->addStretch();

    auto *ratingArea = new QWidget(content);
    ratingArea->setFixedHeight(48);
    auto *ratingAreaLayout = new QHBoxLayout(ratingArea);
    ratingAreaLayout->setContentsMargins(0, 0, 0, 0);
    ratingWidget = new QWidget(ratingArea);
    ratingWidget->setMaximumWidth(700);
    auto *ratingLayout = new QHBoxLayout(ratingWidget);
    ratingLayout->setContentsMargins(0, 0, 0, 0);
    ratingLayout->setSpacing(8);
    againButton = new QPushButton("Nochmal");
    hardButton = new QPushButton("Schwer");
    goodButton = new QPushButton("Gut");
    easyButton = new QPushButton("Einfach");
    for (QPushButton *button : {againButton, hardButton, goodButton, easyButton}) {
        button->setMinimumWidth(120);
        button->setMaximumWidth(160);
        button->setFixedHeight(48);
    }
    ratingLayout->addWidget(againButton);
    ratingLayout->addWidget(hardButton);
    ratingLayout->addWidget(goodButton);
    ratingLayout->addWidget(easyButton);

    metadataWidget = new QFrame(card);
    metadataWidget->setObjectName("flashcardMetadata");
    metadataWidget->setFixedWidth(260);
    metadataWidget->setStyleSheet(R"(
        QFrame#flashcardMetadata {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.10);
            border-radius: 0;
        }
        QFrame#flashcardMetadata QLabel {
            background: transparent;
        }
    )");
    auto *metadataLayout = new QVBoxLayout(metadataWidget);
    metadataLayout->setContentsMargins(10, 7, 10, 7);
    metadataLabel = new QLabel(metadataWidget);
    metadataLabel->setTextFormat(Qt::RichText);
    metadataLabel->setStyleSheet("color: #D0D0D0;");
    metadataLayout->addWidget(metadataLabel);
    metadataWidget->hide();

    cardLayout->addWidget(metadataWidget, 0, Qt::AlignRight);
    auto *actionLayout = new QHBoxLayout();
    commentButton = new QPushButton(card);
    commentButton->setIcon(QIcon(":/assets/icons/speech-bubble.png"));
    commentButton->hide();
    actionLayout->addWidget(commentButton);
    actionLayout->addStretch();
    actionLayout->addWidget(startButton);
    actionLayout->addWidget(showAnswerButton);
    actionLayout->addStretch();
    cardLayout->addLayout(actionLayout);
    layout->addWidget(card, 1);
    ratingAreaLayout->addStretch();
    ratingAreaLayout->addWidget(ratingWidget);
    ratingAreaLayout->addStretch();
    layout->addWidget(ratingArea);
    ratingWidget->hide();

    outerLayout->addStretch();
    outerLayout->addWidget(content, 1);
    outerLayout->addStretch();

    connect(&apiClient, &ApiClient::flashcardsReceived, this,
            [this](const std::vector<FlashcardReviewQueueItem> &receivedFlashcards) {
                flashcards = receivedFlashcards;
                currentIndex = 0;
                showStartScreen();
            });

    connect(startButton, &QPushButton::clicked, this, &LearningView::showCurrentFlashcard);
    connect(pauseButton, &QPushButton::clicked, this, &LearningView::showStartScreen);

    connect(showAnswerButton, &QPushButton::clicked, this, [this] {
        showAnswer();
    });

    connect(commentButton, &QPushButton::clicked, this, [this] {
        showCurrentComments();
    });
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

    connect(againButton, &QPushButton::clicked, this, [this] {
        rateFlashcard(FlashcardRating::Again);
    });
    connect(hardButton, &QPushButton::clicked, this, [this] {
        rateFlashcard(FlashcardRating::Hard);
    });
    connect(goodButton, &QPushButton::clicked, this, [this] {
        rateFlashcard(FlashcardRating::Good);
    });
    connect(easyButton, &QPushButton::clicked, this, [this] {
        rateFlashcard(FlashcardRating::Easy);
    });

    reloadFlashcards();
};

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
    const bool hasRemainingFlashcards = currentIndex < flashcards.size();
    remainingLabel->setText(
        "Verbleibende Karten in der Warteschlange: "
        + QString::number(hasRemainingFlashcards ? flashcards.size() - currentIndex : 0)
    );
    front->setText(hasRemainingFlashcards ? "Bereit zum Lernen?" : "Keine weiteren Lernkarten");
    startButton->setVisible(hasRemainingFlashcards);
    pauseButton->hide();
    showAnswerButton->hide();
    backLabel->hide();
    back->clear();
    metadataWidget->hide();
    ratingWidget->hide();
    commentButton->hide();
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
    front->setText(flashcards[currentIndex].front);
    updateMetadataBanner();
    metadataWidget->show();
    startButton->hide();
    pauseButton->show();
    backLabel->hide();
    back->clear();
    ratingWidget->hide();
    showAnswerButton->show();
    updateCommentButton();

    remainingLabel->setText(
    "Verbleibende Karten in der Warteschlange: " + QString::number(flashcards.size() - currentIndex)
);

    const FlashcardReviewPreview &preview = flashcards[currentIndex].preview;
    againButton->setText("Nochmal\n" + formatInterval(preview.again));
    hardButton->setText("Schwer\n" + formatInterval(preview.hard));
    goodButton->setText("Gut\n" + formatInterval(preview.good));
    easyButton->setText("Einfach\n" + formatInterval(preview.easy));
}

void LearningView::showAnswer() {
    answerOpenedAt = QDateTime::currentDateTime();
    back->setText(flashcards[currentIndex].back);
    ratingWidget->show();
    showAnswerButton->hide();
    backLabel->show();
}

void LearningView::hideEvent(QHideEvent* event) {
    showStartScreen();
    QWidget::hideEvent(event);
}

void LearningView::refreshMetadata() {
    if (currentIndex < flashcards.size() && metadataWidget->isVisible()) {
        updateMetadataBanner();
    }
}

void LearningView::updateMetadataBanner() {
    const FlashcardReviewQueueItem &flashcard = flashcards[currentIndex];
    metadataLabel->setText(QString(
        "<table cellspacing='0' cellpadding='1'>"
        "<tr><td style='color:#969696'>Zustand</td><td style='color:#D0D0D0; padding-left:12px'>%1</td></tr>"
        "<tr><td style='color:#969696'>Retention</td><td style='color:#D0D0D0; padding-left:12px'>%2</td></tr>"
        "<tr><td style='color:#969696'>Stabilität</td><td style='color:#D0D0D0; padding-left:12px'>%3</td></tr>"
        "<tr><td style='color:#969696'>Schwierigkeit</td><td style='color:#D0D0D0; padding-left:12px'>%4</td></tr>"
        "<tr><td style='color:#969696'>Lernschritt</td><td style='color:#D0D0D0; padding-left:12px'>%5</td></tr>"
        "</table>"
    ).arg(
        flashcardStateName(flashcard.reviewStateId).toHtmlEscaped(),
        retentionLevelText(apiClient.metadata(), flashcard.retentionClassId).toHtmlEscaped(),
        metricText(flashcard.stability),
        metricText(flashcard.difficulty),
        QString::number(flashcard.learningStep)
    ));
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
