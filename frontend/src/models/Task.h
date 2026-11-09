#ifndef FRONTEND_TASK_H
#define FRONTEND_TASK_H

#include <QDateTime>
#include <optional>

struct Task {
    int entity_id;
    QDateTime created_at;
    QDateTime updated_at;
    int version;

    std::optional<int> source_entity_id;
    int group_id;

    QString name;
    std::optional<QString> description;

    int state_id;
    int priority_id;

    std::optional<QDateTime> due_at;
    std::optional<QDateTime> completed_at;
};

#endif //FRONTEND_TASK_H
