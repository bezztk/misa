#ifndef FRONTEND_LEARNINGVIEW_H
#define FRONTEND_LEARNINGVIEW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

#include "../api/ApiClient.h"

class LearningView : public QWidget {
public:
    explicit LearningView(QWidget* parent = nullptr);

private:
    std::vector<FlashcardReviewQueueItem> flashcards;
    std::size_t currentIndex = 0;
    QLabel* remainingLabel;
    QLabel* front;
    QLabel* back;
    QWidget* ratingWidget;
    QPushButton* againButton;
    QPushButton* hardButton;
    QPushButton* goodButton;
    QPushButton* easyButton;
    QPushButton* showAnswerButton;

    void showCurrentFlashcard();
    void showAnswer();
    void rateFlashcard(FlashcardRating);

    ApiClient apiClient;

    QDateTime flashcard_shown_at;
    QDateTime opened_answer_at;
};


#endif //FRONTEND_LEARNINGVIEW_H
