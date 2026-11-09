#include "ApiClient.h"
#include "../models/Task.h"
#include <QNetworkReply>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ApiClient::ApiClient() = default;

QUrl ApiClient::buildUrl(const QString &path) {
    QUrl url;

    url.setScheme(SCHEME);
    url.setHost(HOST);
    url.setPort(PORT);
    url.setPath(QString(BASE_PATH) + path);

    return url;
}

void ApiClient::getTasks() {
    const QNetworkRequest networkRequest(buildUrl(PATH_TASKS));

    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        QJsonArray jsonArray = jsonDoc.array();
        std::vector<Task> tasks;

        for (const QJsonValue &value: jsonArray) {
            QJsonObject object = value.toObject();

            Task task;

            task.entity_id = object["entity_id"].toInt();
            task.name = object["name"].toString();
            task.group_id = object["group_id"].toInt();
            task.state_id = object["state_id"].toInt();
            task.priority_id = object["priority_id"].toInt();

            if (!object["description"].isNull()) {
                task.description = object["description"].toString();
            }

            task.created_at = QDateTime::fromString(
                object["created_at"].toString(),
                Qt::ISODateWithMs
            );

            if (!object["due_at"].isNull()) {
                task.due_at = QDateTime::fromString(
                    object["due_at"].toString(),
                    Qt::ISODateWithMs
                );
            }

            tasks.push_back(task);
        }

        emit taskReceived(tasks);

        reply->deleteLater();
    });
}
