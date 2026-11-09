#include "LearningView.h"

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

    remainingLabel = new QLabel(this);
    layout->addWidget(remainingLabel);

    front = new QLabel(this);
    back = new QLabel(this);
    showAnswerButton = new QPushButton("Antwort anzeigen", this);

    layout->addWidget(front);
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
                showCurrentFlashcard();
            });

    connect(showAnswerButton, &QPushButton::clicked, this, [this] {
        showAnswer();
    });

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

    apiClient.getDueFlashcards();
};

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
        front->setText("Keine weiteren Lernkarten");
        back->clear();
        return;
    }

    flashcard_shown_at = QDateTime::currentDateTime();
    front->setText(flashcards[currentIndex].front);
    back->clear();
    ratingWidget->hide();
    showAnswerButton->show();

    remainingLabel->setText(
    "Verbleibend: " + QString::number(flashcards.size() - currentIndex)
);

    apiClient.getFlashcardPreview(flashcards[currentIndex].id);
}

void LearningView::showAnswer() {
    opened_answer_at = QDateTime::currentDateTime();
    back->setText(flashcards[currentIndex].back);
    ratingWidget->show();
    showAnswerButton->hide();
}
