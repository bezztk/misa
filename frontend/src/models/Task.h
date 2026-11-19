#ifndef FRONTEND_TASK_H
#define FRONTEND_TASK_H

#include <QString>
#include <optional>
#include <vector>

#include "Comment.h"

enum class TaskField {
    Name = 1,
    StateId = 2,
    PriorityId = 3
};

struct Task {
    int id;
    std::optional<int> groupId;
    std::optional<int> domainNodeId;
    QString name;
    int stateId;
    int priorityId;
    std::vector<Comment> comments;
};

struct CreateTask {
    QString name;
    std::optional<int> groupId;
    int priorityId;
    std::optional<int> domainNodeId;
};

#endif //FRONTEND_TASK_H
