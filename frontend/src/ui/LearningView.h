#ifndef FRONTEND_LEARNINGVIEW_H
#define FRONTEND_LEARNINGVIEW_H

#include <array>

#include <QDateTime>
#include <QWidget>

#include "../api/ApiClient.h"

class QHideEvent;
class QLabel;
class QMenu;
class QProgressBar;
class QPushButton;
class QScrollArea;
class QShortcut;
class QStackedWidget;

class LearningView : public QWidget {
public:
    explicit LearningView(ApiClient &apiClient, QWidget* parent = nullptr);
    void refreshMetadata();
    void reloadFlashcards();

protected:
    void hideEvent(QHideEvent* event) override;

private:
    struct RatingControl {
        QPushButton *button;
        QShortcut *shortcut;
    };

    std::vector<FlashcardReviewQueueItem> flashcards;
    std::size_t currentIndex = 0;
    QStackedWidget* screens;
    QWidget* startScreen;
    QWidget* sessionScreen;
    QLabel* dueCountLabel;
    QLabel* dueBreakdown;
    QLabel* remainingLabel;
    QLabel* stateLabel;
    QProgressBar* progress;
    QScrollArea* cardScrollArea;
    QLabel* front;
    QLabel* hint;
    QLabel* back;
    QPushButton* revealHint;
    QPushButton* revealAnswer;
    QShortcut* hintShortcut;
    QShortcut* answerShortcut;
    QMenu* metadataMenu;
    std::array<QLabel *, 5> metadataValues;
    QWidget* ratingWidget;
    std::array<RatingControl, 4> ratingControls;
    QPushButton* startButton;
    QWidget* learningCard;
    QPushButton* commentButton;

    QWidget *createStartScreen();
    QWidget *createSessionScreen();
    void showStartScreen();
    void showCurrentFlashcard();
    void showHint();
    void showAnswer();
    void rateFlashcard(FlashcardRating);
    void updateSessionActions();
    void updateMetadata();
    void showCurrentComments();
    void updateCommentButton();

    ApiClient &apiClient;
    bool ratingPending = false;

    QDateTime flashcardShownAt;
    QDateTime answerOpenedAt;
};


#endif //FRONTEND_LEARNINGVIEW_H
