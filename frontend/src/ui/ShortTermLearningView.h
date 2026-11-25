#ifndef FRONTEND_SHORTTERMLEARNINGVIEW_H
#define FRONTEND_SHORTTERMLEARNINGVIEW_H

#include "../models/ShortTermLearning.h"

#include <QWidget>

#include <vector>

class ApiClient;
class QHideEvent;
class QJsonArray;
class QLabel;
class QListWidget;
class QProgressBar;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QShortcut;
class QShowEvent;
class QStackedWidget;
class QTableWidget;
class QTimer;

class ShortTermLearningView final : public QWidget {
public:
    explicit ShortTermLearningView(ApiClient &apiClient, QWidget *parent = nullptr);
    void openCreateSetDialog();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

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
    void centerSetGrid();
    void loadCards();
    void renderCards();
    void updateStartButton();
    void showOverview();
    void openSet(const ShortTermLearningSet &set);
    void showSetMenu(int setId, const QPoint &globalPosition);
    void refreshAfterSetChange(bool saved);
    void resetSet(int setId);
    void deleteSet(int setId);
    void importCards(int setId);
    void saveCards(int setId, const QJsonArray &entries);
    void createCard(int setId);
    void editCard(int row);
    void deleteCard(int row);
    void startSession();
    void endSession();
    void advanceSession(bool starting = false);
    void showQuestion(Question next);
    void rateQuestion(ShortTermLearningRating rating);

    ApiClient &apiClient;
    QStackedWidget *screens;
    QWidget *overview;
    QWidget *details;
    QWidget *session;
    QListWidget *setsGrid;
    QTableWidget *cardsTable;
    QLabel *setTitle;
    QPushButton *startButton;
    QLabel *remaining;
    QProgressBar *sessionProgress;
    QStackedWidget *sessionContent;
    QScrollArea *cardScroll;
    QLabel *countdown;
    QTimer *countdownTimer;
    QLabel *question;
    QLabel *hint;
    QLabel *answer;
    QPushButton *revealHint;
    QPushButton *revealAnswer;
    QWidget *ratingActions;
    QShortcut *hintShortcut;
    QShortcut *answerShortcut;
    std::array<QPushButton *, 4> ratingButtons;
    std::array<QShortcut *, 4> ratingShortcuts;

    std::vector<ShortTermLearningSet> sets;
    ShortTermLearningSet selectedSet{};
    std::vector<ShortTermLearningCard> cards;
    std::array<ShortTermLearningRatingOption, 4> ratingOptions;
    Question currentQuestion{};
    bool busy = false;
    bool createSetPending = false;
};

#endif // FRONTEND_SHORTTERMLEARNINGVIEW_H
