#ifndef FRONTEND_TASK_H
#define FRONTEND_TASK_H

#include <QString>

struct Task {
    int id;
    int group_id;
    QString name;
    int state_id;
    int priority_id;
};

struct CreateTask {
    QString name;
    int group_id;
    int priority_id;
};

struct UpdateTask {
    QString name;
    int state_id;
    int priority_id;
};

#endif //FRONTEND_TASK_H
