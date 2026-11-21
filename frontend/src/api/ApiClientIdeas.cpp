#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

void ApiClient::getIdeas() {
    QNetworkReply *reply = manager.get(QNetworkRequest(buildUrl(PATH_IDEAS)));

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (!readJson(reply, "Ideen laden", response) || !response.isArray()) {
            if (!response.isNull() && !response.isArray()) {
                emit requestFailed("Ideen laden: Unerwartetes Datenformat");
            }
            reply->deleteLater();
            return;
        }

        std::vector<Idea> ideas;
        for (const QJsonValue &value : response.array()) {
            const QJsonObject object = value.toObject();
            Idea idea{
                .id = object["id"].toInt(),
                .title = object["title"].toString(),
                .content = object["content"].toString(),
                .phaseId = object["phase_id"].toInt(),
                .stateId = object["state_id"].toInt(),
                .createdAt = QDateTime::fromString(
                    object["created_at"].toString(), Qt::ISODate
                ),
                .comments = parseComments(object["comments"].toArray())
            };
            if (!object["completed_at"].isNull()) {
                idea.completedAt = QDateTime::fromString(
                    object["completed_at"].toString(), Qt::ISODate
                );
            }
            ideas.push_back(std::move(idea));
        }

        emit ideasReceived(ideas);
        reply->deleteLater();
    });
}

void ApiClient::createIdea(const CreateIdea &idea) {
    QNetworkRequest request(buildUrl(PATH_IDEAS));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.post(
        request,
        QJsonDocument(QJsonObject{
            {"title", idea.title},
            {"content", idea.content}
        }).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit ideaCreated();
        }
        reply->deleteLater();
    });
}

void ApiClient::updateIdeaField(
    const int id,
    const IdeaField field,
    const QJsonValue &value
) {
    QNetworkRequest request(buildUrl(
        "/ideas/" + QString::number(id)
        + "/fields/" + QString::number(static_cast<int>(field))
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(QJsonObject{{"value", value}}).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit ideaUpdated();
        }
        reply->deleteLater();
    });
}

void ApiClient::deleteIdea(const int id) {
    QNetworkReply *reply = manager.deleteResource(QNetworkRequest(buildUrl(
        "/ideas/" + QString::number(id)
    )));

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit ideaDeleted();
        }
        reply->deleteLater();
    });
}
