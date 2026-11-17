#ifndef FRONTEND_LEARNINGVIEW_H
#define FRONTEND_LEARNINGVIEW_H

#include <QDateTime>
#include <QWidget>

#include "../api/ApiClient.h"

class QHideEvent;
class QLabel;
class QPushButton;

class LearningView : public QWidget {
public:
    explicit LearningView(ApiClient &apiClient, QWidget* parent = nullptr);
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

    ApiClient &apiClient;
    bool ratingPending = false;

    QDateTime flashcardShownAt;
    QDateTime answerOpenedAt;
};


#endif //FRONTEND_LEARNINGVIEW_H
