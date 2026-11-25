#ifndef FRONTEND_SHORTTERMLEARNINGVIEW_H
#define FRONTEND_SHORTTERMLEARNINGVIEW_H

#include "../models/ShortTermLearning.h"

#include <QWidget>

#include <deque>
#include <vector>

class ApiClient;
class QHideEvent;
class QLabel;
class QProgressBar;
class QPushButton;
class QScrollArea;
class QShortcut;
class QShowEvent;
class QStackedWidget;
class QTableWidget;

class ShortTermLearningView final : public QWidget {
public:
    explicit ShortTermLearningView(ApiClient &apiClient, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    struct Question {
        std::size_t cardIndex;
        bool reverse;
    };

    QWidget *createOverview();
    QWidget *createDetails();
    QWidget *createSession();
    void setBusy(bool busy);
    void updateSessionActions();
    void loadSets();
    void loadCards();
    void renderCards();
    void createSet();
    void openSet();
    void resetSet();
    void deleteSet();
    void importCards();
    void editCard();
    void deleteCard();
    void startSession();
    void endSession();
    void showQuestion();
    void rateQuestion(ShortTermLearningState state);

    ApiClient &apiClient;
    QStackedWidget *screens;
    QWidget *overview;
    QWidget *details;
    QWidget *session;
    QTableWidget *setsTable;
    QTableWidget *cardsTable;
    QLabel *setTitle;
    QPushButton *startButton;
    QLabel *remaining;
    QProgressBar *sessionProgress;
    QScrollArea *cardScroll;
    QLabel *question;
    QLabel *hint;
    QLabel *answer;
    QPushButton *revealHint;
    QPushButton *revealAnswer;
    QWidget *ratingActions;
    QShortcut *hintShortcut;
    QShortcut *answerShortcut;
    QShortcut *continueShortcut;
    QShortcut *completeShortcut;

    std::vector<ShortTermLearningSet> sets;
    ShortTermLearningSet selectedSet{};
    std::vector<ShortTermLearningCard> cards;
    std::deque<Question> queue;
    bool busy = false;
};

#endif // FRONTEND_SHORTTERMLEARNINGVIEW_H
