#ifndef FRONTEND_DOMAINNODE_H
#define FRONTEND_DOMAINNODE_H

#include <QString>

#include <optional>
#include <vector>

#include "Comment.h"

enum class DomainNodeField {
    Name = 1,
    KnowledgeLevelId = 2,
    CompletenessLevelId = 3,
    FlashcardStatusId = 4,
    VerificationStatusId = 5,
    VerificationLevelId = 6
};

struct DomainNode {
    int id;
    int domainId;
    std::optional<int> parentId{};
    std::optional<int> knowledgeLevelId{};
    QString name;
    bool hasDocument;
    int flashcardCount;
    int attachmentCount;
    int pursuitCount;
    int taskCount;
    std::optional<int> completenessLevelId{};
    std::optional<int> flashcardStatusId{};
    std::optional<int> verificationStatusId{};
    std::optional<int> verificationLevelId{};
    std::vector<Comment> comments;
};

#endif // FRONTEND_DOMAINNODE_H
