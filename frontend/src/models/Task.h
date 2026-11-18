#ifndef FRONTEND_TASK_H
#define FRONTEND_TASK_H

#include <QString>
#include <optional>

struct Task {
    int id;
    std::optional<int> groupId;
    std::optional<int> domainNodeId;
    QString name;
    int stateId;
    int priorityId;
};

struct CreateTask {
    QString name;
    std::optional<int> groupId;
    int priorityId;
    std::optional<int> domainNodeId;
};

struct UpdateTask {
    QString name;
    int stateId;
    int priorityId;
};

#endif //FRONTEND_TASK_H
