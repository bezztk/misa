#ifndef FRONTEND_TASKVIEW_H
#define FRONTEND_TASKVIEW_H

#include "../api/ApiClient.h"
#include "../models/Task.h"

#include <QWidget>

#include <vector>

class QListWidget;
class QTreeWidget;

class TaskView : public QWidget {
public:
    explicit TaskView(ApiClient &apiClient, QWidget *parent = nullptr);
    void renderTasks();
    void refreshTasks();
    void refreshMetadata();

private:
    void renderNavigation();
    void openCreateDialog();
    void openEditDialog(const Task &task);
    void openScheduleDialog(const Task &task);
    const Task *taskById(int id) const;

    std::vector<Task> tasks;
    std::vector<TaskPursuit> pursuits;
    ApiClient &apiClient;
    QListWidget *navigation;
    QTreeWidget *taskList;
    int selectedNavigationType = 0;
    int selectedNavigationId = 0;
};

#endif // FRONTEND_TASKVIEW_H
