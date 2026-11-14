#include "LearningView.h"

#include "../models/AppState.h"

#include <QHideEvent>
#include <QFrame>
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

static QString flashcardStateName(const int state) {
    switch (state) {
        case 1: return "Lernen";
        case 2: return "Wiederholen";
        case 3: return "Erneut lernen";
        default: return "Neu";
    }
}

static QString retentionLevelText(const int id) {
    for (const FlashcardRetentionClass &level : AppState::metadata.flashcard_retention_classes) {
        if (level.id == id) {
            return QString("%1 (%2 %)")
                .arg(level.display_name.value_or(level.name))
                .arg(qRound(level.desired_retention * 100));
        }
    }

    return "#" + QString::number(id);
}

static QString metricText(const std::optional<float> value) {
    return value ? QString::number(*value, 'f', 2) : "–";
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

    metadataWidget = new QFrame(this);
    metadataWidget->setObjectName("flashcardMetadata");
    metadataWidget->setFixedWidth(260);
    metadataWidget->setStyleSheet(R"(
        QFrame#flashcardMetadata {
            background: rgba(255, 255, 255, 0.05);
            border-left: 3px solid #64B5F6;
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

    layout->addStretch();
    layout->addWidget(metadataWidget, 0, Qt::AlignRight);
    layout->addSpacing(6);
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
    showStartScreen();
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
    updateMetadataBanner();
    metadataWidget->show();
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

void LearningView::refreshMetadata() {
    if (currentIndex < flashcards.size() && metadataWidget->isVisible()) {
        updateMetadataBanner();
    }
}

void LearningView::updateMetadataBanner() {
    const FlashcardReviewQueueItem &flashcard = flashcards[currentIndex];
    metadataLabel->setText(QString(
        "<table cellspacing='0' cellpadding='1'>"
        "<tr><td style='color:#777777'>Zustand</td><td style='color:#D0D0D0; padding-left:12px'>%1</td></tr>"
        "<tr><td style='color:#777777'>Retention</td><td style='color:#D0D0D0; padding-left:12px'>%2</td></tr>"
        "<tr><td style='color:#777777'>Stabilität</td><td style='color:#D0D0D0; padding-left:12px'>%3</td></tr>"
        "<tr><td style='color:#777777'>Schwierigkeit</td><td style='color:#D0D0D0; padding-left:12px'>%4</td></tr>"
        "<tr><td style='color:#777777'>Lernschritt</td><td style='color:#D0D0D0; padding-left:12px'>%5</td></tr>"
        "</table>"
    ).arg(
        flashcardStateName(flashcard.state).toHtmlEscaped(),
        retentionLevelText(flashcard.retention_levels_id).toHtmlEscaped(),
        metricText(flashcard.stability),
        metricText(flashcard.difficulty),
        QString::number(flashcard.learning_step)
    ));
}
