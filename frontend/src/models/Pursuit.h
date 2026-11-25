#ifndef FRONTEND_PURSUIT_H
#define FRONTEND_PURSUIT_H

#include <QString>

#include <optional>
#include <vector>

#include "Comment.h"

struct PursuitRequirement {
    int id;
    int requirementTypeId;
    std::optional<int> knowledgeLevelId;
    std::optional<int> completenessLevelId;
    std::optional<int> flashcardStatusId;
    std::optional<int> verificationStatusId;
    std::optional<int> verificationLevelId;
};

struct Pursuit {
    int id;
    int domainNodeId;
    QString name;
    std::vector<PursuitRequirement> requirements{};
    std::vector<Comment> comments;
};

struct UpdatePursuitRequirements {
    std::optional<int> knowledgeLevelId;
    std::optional<int> completenessLevelId;
    std::optional<int> flashcardStatusId;
    std::optional<int> verificationStatusId;
    std::optional<int> verificationLevelId;
};

#endif // FRONTEND_PURSUIT_H
