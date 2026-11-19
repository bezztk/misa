#ifndef FRONTEND_COMMENT_H
#define FRONTEND_COMMENT_H

#include <QDateTime>
#include <QString>

#include <vector>

class QJsonArray;
class QJsonObject;

struct Comment {
    int id;
    int entityId;
    QString content;
    QDateTime createdAt;
    QDateTime updatedAt;
};

Comment parseComment(const QJsonObject &object);
std::vector<Comment> parseComments(const QJsonArray &array);

#endif // FRONTEND_COMMENT_H
