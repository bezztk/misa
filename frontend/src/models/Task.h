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
    std::vector<int> contextIds;
    std::optional<int> domainNodeId;
    QString name;
    int stateId;
    int priorityId;
    std::vector<Comment> comments;

    bool isOpen() const {
        return stateId == 1 || stateId == 2;
    }
};

struct CreateTask {
    QString name;
    std::vector<int> contextIds;
    int priorityId;
    std::optional<int> domainNodeId;
};

#endif //FRONTEND_TASK_H
