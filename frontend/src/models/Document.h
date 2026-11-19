#ifndef FRONTEND_DOCUMENT_H
#define FRONTEND_DOCUMENT_H

#include <QDateTime>
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

struct DocumentVersion {
    int version;
    QString content;
    QDateTime capturedAt;
};

#endif // FRONTEND_DOCUMENT_H
