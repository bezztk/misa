#ifndef FRONTEND_LEARNINGVIEW_H
#define FRONTEND_LEARNINGVIEW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

#include "../api/ApiClient.h"

class QHideEvent;

class LearningView : public QWidget {
public:
    explicit LearningView(QWidget* parent = nullptr);
    void refreshMetadata();

protected:
    void hideEvent(QHideEvent* event) override;

private:
    std::vector<FlashcardReviewQueueItem> flashcards;
    std::size_t currentIndex = 0;
    QLabel* remainingLabel;
    QLabel* front;
    QLabel* backLabel;
    QLabel* back;
    QWidget* metadataWidget;
    QLabel* metadataLabel;
    QWidget* ratingWidget;
    QPushButton* againButton;
    QPushButton* hardButton;
    QPushButton* goodButton;
    QPushButton* easyButton;
    QPushButton* showAnswerButton;
    QPushButton* startButton;
    QPushButton* pauseButton;

    void reloadFlashcards();
    void showStartScreen();
    void showCurrentFlashcard();
    void showAnswer();
    void rateFlashcard(FlashcardRating);
    void updateMetadataBanner();

    ApiClient apiClient;

    QDateTime flashcard_shown_at;
    QDateTime opened_answer_at;
};


#endif //FRONTEND_LEARNINGVIEW_H
