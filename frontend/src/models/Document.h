#ifndef FRONTEND_DOCUMENT_H
#define FRONTEND_DOCUMENT_H

#include <QString>
#include <optional>

struct Document {
    int domainNodeId;
    QString content;
    std::optional<int> completenessLevelId{};
    std::optional<int> flashcardStatusId{};
    std::optional<int> verificationStatusId{};
    std::optional<int> verificationLevelId{};
};

#endif // FRONTEND_DOCUMENT_H
