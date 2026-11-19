#include "Comment.h"

#include <QJsonArray>
#include <QJsonObject>

Comment parseComment(const QJsonObject &object) {
    return {
        .id = object["id"].toInt(),
        .entityId = object["entity_id"].toInt(),
        .content = object["content"].toString(),
        .createdAt = QDateTime::fromString(object["created_at"].toString(), Qt::ISODate),
        .updatedAt = QDateTime::fromString(object["updated_at"].toString(), Qt::ISODate)
    };
}

std::vector<Comment> parseComments(const QJsonArray &array) {
    std::vector<Comment> comments;
    comments.reserve(array.size());
    for (const QJsonValue &value : array) {
        comments.push_back(parseComment(value.toObject()));
    }
    return comments;
}
