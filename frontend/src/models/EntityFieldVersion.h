#ifndef FRONTEND_ENTITYFIELDVERSION_H
#define FRONTEND_ENTITYFIELDVERSION_H

#include <QDateTime>
#include <QJsonValue>
#include <QString>

struct EntityFieldVersion {
    int version;
    QString fieldName;
    QJsonValue oldValue;
    QJsonValue newValue;
    QDateTime changedAt;
};

#endif // FRONTEND_ENTITYFIELDVERSION_H
