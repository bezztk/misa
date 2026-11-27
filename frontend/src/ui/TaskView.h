#ifndef FRONTEND_TASKVIEW_H
#define FRONTEND_TASKVIEW_H

#include "../models/Context.h"
#include "../models/Task.h"

#include <QWidget>

class ApiClient;
class QLineEdit;
class QListWidget;

class TaskView : public QWidget {
public:
    explicit TaskView(ApiClient &apiClient, QWidget *parent = nullptr);
    void openCreateDialog();

private:
    void renderNavigation();
    void updateNavigationCounts();
    void renderTasks();
    int selectedContextId() const;
    const Task &taskById(int id) const;
    void openScheduleDialog(const Task &task, bool isReminder);
    void showTaskComments(const Task &task);
    void showTaskMenu(const QPoint &position);

    ApiClient &apiClient;
    QListWidget *navigation;
    QLineEdit *search;
    QListWidget *taskList;
    std::vector<Context> contexts;
    std::vector<Task> tasks;
};

#endif // FRONTEND_TASKVIEW_H
