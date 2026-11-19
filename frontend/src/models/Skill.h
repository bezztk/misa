#ifndef FRONTEND_SKILL_H
#define FRONTEND_SKILL_H

#include <QString>

#include <optional>
#include <vector>

#include "Comment.h"

enum class SkillField {
    Name = 1,
    PsychomotorLevelId = 2
};

struct Skill {
    int id;
    int domainNodeId;
    std::optional<int> parentSkillId{};
    QString name;
    std::optional<int> psychomotorLevelId{};
    std::vector<Comment> comments;
};

#endif // FRONTEND_SKILL_H
