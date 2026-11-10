#include "LearningView.h"

#include <QHideEvent>
#include <QHBoxLayout>
#include <QLabel>
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

LearningView::LearningView(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* headerLayout = new QHBoxLayout();
    auto* refreshButton = new QPushButton("Neu laden", this);
    pauseButton = new QPushButton("Pause", this);
    pauseButton->hide();

    remainingLabel = new QLabel(this);
    remainingLabel->setStyleSheet(R"(
    color: #666666;
    font-size: 11px;
    font-weight: 300;
)");
    headerLayout->addWidget(remainingLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(pauseButton);
    headerLayout->addWidget(refreshButton);
    layout->addLayout(headerLayout);
    layout->addSpacing(6);

    front = new QLabel(this);
    front->setText("Lernkarten werden geladen");
    backLabel = new QLabel("Antwort:");
    backLabel->hide();
    back = new QLabel(this);

    showAnswerButton = new QPushButton("Antwort anzeigen", this);
    showAnswerButton->hide();
    startButton = new QPushButton("Starten", this);
    startButton->hide();

    layout->addWidget(front);
    layout->addSpacing(20);
    layout->addWidget(startButton);
    layout->addWidget(backLabel);
    layout->addWidget(back);
    layout->addWidget(showAnswerButton);

    ratingWidget = new QWidget(this);
    auto *ratingLayout = new QHBoxLayout(ratingWidget);
    againButton = new QPushButton("Nochmal");
    hardButton = new QPushButton("Schwer");
    goodButton = new QPushButton("Gut");
    easyButton = new QPushButton("Einfach");
    ratingLayout->addWidget(againButton);
    ratingLayout->addWidget(hardButton);
    ratingLayout->addWidget(goodButton);
    ratingLayout->addWidget(easyButton);

    layout->addStretch();
    layout->addWidget(ratingWidget);
    ratingWidget->hide();

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

    connect(refreshButton, &QPushButton::clicked, this, &LearningView::reloadFlashcards);

    connect(&apiClient, &ApiClient::flashcardRated, this, [this] {
        ++currentIndex;
        showCurrentFlashcard();
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

    connect(&apiClient, &ApiClient::flashcardPreviewCalculated, this,
    [this](const FlashcardReviewPreview& preview) {
        againButton->setText("Nochmal\n" + formatInterval(preview.again));
        hardButton->setText("Schwer\n" + formatInterval(preview.hard));
        goodButton->setText("Gut\n" + formatInterval(preview.good));
        easyButton->setText("Einfach\n" + formatInterval(preview.easy));
    });

    reloadFlashcards();
};

void LearningView::reloadFlashcards() {
    flashcards.clear();
    currentIndex = 0;
    remainingLabel->setText("Lernkarten werden neu geladen");
    front->setText("Lernkarten werden geladen");
    startButton->hide();
    pauseButton->hide();
    showAnswerButton->hide();
    backLabel->hide();
    back->clear();
    ratingWidget->hide();
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
    ratingWidget->hide();
}

void LearningView::rateFlashcard(FlashcardRating rating) {
    const auto ms = static_cast<int>(flashcard_shown_at.msecsTo(opened_answer_at));

    apiClient.rateFlashcard(
        flashcards[currentIndex].id,
        ReviewFlashcard{
            .rating = static_cast<int16_t>(rating),
            .duration_ms = ms
        });
}

void LearningView::showCurrentFlashcard() {
    if (currentIndex >= flashcards.size()) {
        showStartScreen();
        return;
    }

    flashcard_shown_at = QDateTime::currentDateTime();
    front->setText(flashcards[currentIndex].front);
    startButton->hide();
    pauseButton->show();
    backLabel->hide();
    back->clear();
    ratingWidget->hide();
    showAnswerButton->show();

    remainingLabel->setText(
    "Verbleibende Karten in der Warteschlange: " + QString::number(flashcards.size() - currentIndex)
);

    apiClient.getFlashcardPreview(flashcards[currentIndex].id);
}

void LearningView::showAnswer() {
    opened_answer_at = QDateTime::currentDateTime();
    back->setText(flashcards[currentIndex].back);
    ratingWidget->show();
    showAnswerButton->hide();
    backLabel->show();
}

void LearningView::hideEvent(QHideEvent* event) {
    showStartScreen();
    QWidget::hideEvent(event);
}
