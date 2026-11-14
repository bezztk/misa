#ifndef FRONTEND_SKILL_H
#define FRONTEND_SKILL_H

#include <QString>

#include <optional>

struct Skill {
    int id;
    int domainNodeId;
    std::optional<int> parentSkillId;
    QString name;
    std::optional<int> psychomotorLevelId;
};

#endif // FRONTEND_SKILL_H
