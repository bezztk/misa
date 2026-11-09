#ifndef FRONTEND_TASKVIEW_H
#define FRONTEND_TASKVIEW_H

#include "../api/ApiClient.h"
#include "../models/Task.h"

#include <vector>
#include <QWidget>

class TaskView : public QWidget {
public:
    explicit TaskView(QWidget* parent = nullptr);

private:
    std::vector<Task> tasks;
    ApiClient apiClient;
};


#endif //FRONTEND_TASKVIEW_H
