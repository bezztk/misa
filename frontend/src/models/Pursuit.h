#ifndef FRONTEND_PURSUIT_H
#define FRONTEND_PURSUIT_H

#include <QString>

#include <optional>
#include <vector>

struct PursuitRequirement {
    int id;
    int requirementTypeId;
    std::optional<int> skillId;
    std::optional<int> cognitiveLevelId;
    std::optional<int> psychomotorLevelId;
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
};

struct PursuitSkillRequirement {
    int skillId;
    int psychomotorLevelId;
};

struct UpdatePursuitRequirements {
    std::optional<int> cognitiveLevelId;
    std::optional<int> completenessLevelId;
    std::optional<int> flashcardStatusId;
    std::optional<int> verificationStatusId;
    std::optional<int> verificationLevelId;
    std::vector<PursuitSkillRequirement> skills;
};

#endif // FRONTEND_PURSUIT_H
