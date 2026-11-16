#ifndef FRONTEND_TASK_H
#define FRONTEND_TASK_H

#include <QString>
#include <optional>

struct Task {
    int id;
    std::optional<int> group_id;
    std::optional<int> pursuit_id;
    QString name;
    int state_id;
    int priority_id;
};

struct CreateTask {
    QString name;
    std::optional<int> group_id;
    int priority_id;
    std::optional<int> pursuit_id;
};

struct TaskPursuit {
    int id;
    QString name;
};

struct UpdateTask {
    QString name;
    int state_id;
    int priority_id;
};

#endif //FRONTEND_TASK_H
