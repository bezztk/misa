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

void ApiClient::getDueFlashcards() {
    const QNetworkRequest networkRequest(buildUrl(PATH_FLASHCARDS));

    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        std::vector<FlashcardReviewQueueItem> flashcards;

        const QByteArray data = reply->readAll();
        const QJsonArray jsonArray = QJsonDocument::fromJson(data).array();

        for (const QJsonValue &value: jsonArray) {
            FlashcardReviewQueueItem flashcard;
            QJsonObject object = value.toObject();

            flashcard.id = object["id"].toInt();
            flashcard.front = object["front"].toString();
            flashcard.back = object["back"].toString();

            flashcards.push_back(flashcard);
        }

        emit flashcardsReceived(flashcards);

        reply->deleteLater();
    });
}

void ApiClient::rateFlashcard(const int id, const ReviewFlashcard reviewFlashcard) {
    QNetworkRequest networkRequest(buildUrl("/flashcards/" + QString::number(id) + "/reviews"));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["rating"] = reviewFlashcard.rating;
    json["duration_ms"] = reviewFlashcard.duration_ms;

    QNetworkReply *reply = manager.post(networkRequest, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 201) {
            emit flashcardRated();
        } else {
            qWarning() << "Flashcard review failed: " << reply->errorString();
        }

        reply->deleteLater();
    });
}

void ApiClient::getFlashcardPreview(const int id) {
    const QNetworkRequest networkRequest(buildUrl("/flashcards/" + QString::number(id) + "/preview"));

    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        FlashcardReviewPreview preview{};
        const QByteArray data = reply->readAll();
        const QJsonObject jsonObject = QJsonDocument::fromJson(data).object();

        preview.again = static_cast<float>(jsonObject["again"].toDouble());
        preview.hard = static_cast<float>(jsonObject["hard"].toDouble());
        preview.good = static_cast<float>(jsonObject["good"].toDouble());
        preview.easy = static_cast<float>(jsonObject["easy"].toDouble());

        emit flashcardPreviewCalculated(preview);

        reply->deleteLater();
    });
}

void ApiClient::getTasks() {
    const QNetworkRequest networkRequest(buildUrl(PATH_TASKS));

    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        std::vector<Task> tasks;

        const QByteArray data = reply->readAll();
        const QJsonArray jsonArray = QJsonDocument::fromJson(data).array();

        for (const QJsonValue &value: jsonArray) {
            Task task;
            QJsonObject object = value.toObject();

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
