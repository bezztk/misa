#ifndef FRONTEND_SOURCE_H
#define FRONTEND_SOURCE_H

#include <QByteArray>
#include <QString>

struct Source {
    QString id;
    QString title;
    QString url;
    QString resourceType;
    QString description;
    int resourceTypeId;
    QByteArray favicon;
};

#endif // FRONTEND_SOURCE_H
