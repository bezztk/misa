#ifndef FRONTEND_ATTACHMENT_H
#define FRONTEND_ATTACHMENT_H

#include <QString>
#include <QtTypes>

struct Attachment {
    int id;
    int domainNodeId;
    QString originalName;
    QString storageName;
    qint64 sizeBytes;
    QString createdAt;
};

#endif // FRONTEND_ATTACHMENT_H
