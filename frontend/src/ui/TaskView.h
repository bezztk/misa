#ifndef FRONTEND_TASKVIEW_H
#define FRONTEND_TASKVIEW_H

#include "../api/ApiClient.h"
#include "../models/Task.h"

#include <vector>
#include <QWidget>

class QVBoxLayout;

class TaskView : public QWidget {
public:
    explicit TaskView(QWidget* parent = nullptr);
    void renderTasks();

private:
    void openEditDialog(const Task& task);

    std::vector<Task> tasks;
    ApiClient apiClient;
    QVBoxLayout* cardsLayout;
};


#endif //FRONTEND_TASKVIEW_H
