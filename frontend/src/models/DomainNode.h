#ifndef FRONTEND_DOMAINNODE_H
#define FRONTEND_DOMAINNODE_H

#include <QString>

#include <optional>

struct DomainNode {
    int id;
    int domainId;
    std::optional<int> parentId;
    std::optional<int> cognitiveLevelId;
    QString name;
    bool hasDocument;
    int flashcardCount;
    int skillCount;
    std::optional<int> completenessLevelId;
    std::optional<int> flashcardStatusId;
    std::optional<int> verificationStatusId;
    std::optional<int> verificationLevelId;
};

#endif // FRONTEND_DOMAINNODE_H
