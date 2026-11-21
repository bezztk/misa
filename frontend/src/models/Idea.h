#ifndef FRONTEND_IDEA_H
#define FRONTEND_IDEA_H

#include <QDateTime>
#include <QString>

#include <optional>
#include <vector>

#include "Comment.h"

enum class IdeaField {
    Title = 1,
    Content = 2,
    PhaseId = 3,
    StateId = 4
};

struct Idea {
    int id;
    QString title;
    QString content;
    int phaseId;
    int stateId;
    QDateTime createdAt;
    std::optional<QDateTime> completedAt;
    std::vector<Comment> comments;
};

struct CreateIdea {
    QString title;
    QString content;
};

#endif // FRONTEND_IDEA_H
