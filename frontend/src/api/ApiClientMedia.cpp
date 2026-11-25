#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <utility>

int ApiClient::getMedia() {
    const int requestId = ++mediaRequestSequence;
    QNetworkRequest request(buildUrl(PATH_MEDIA));
    request.setAttribute(HANDLE_ERROR_LOCALLY, true);
    QNetworkReply *reply = manager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId] {
        QJsonDocument response;
        if (readJson(reply, "Medien laden", response) && response.isArray()) {
            std::vector<Media> media;
            for (const QJsonValue &value : response.array()) {
                const QJsonObject object = value.toObject();
                const QJsonValue rating = object["personal_rating"];
                const QJsonValue runtime = object["runtime_minutes"];
                QStringList genres;
                for (const QJsonValue &genre : object["genres"].toArray()) {
                    genres.append(genre.toString());
                }
                media.push_back({
                    .entityId = object["entity_id"].toInt(),
                    .tmdbId = object["tmdb_id"].toInt(),
                    .mediaType = object["media_type"].toString(),
                    .title = object["title"].toString(),
                    .originalTitle = object["original_title"].toString(),
                    .releaseDate = QDate::fromString(object["release_date"].toString(), Qt::ISODate),
                    .posterPath = object["poster_path"].toString(),
                    .watchStatusId = object["watch_status_id"].toInt(),
                    .personalRating = rating.isDouble() ? std::optional(rating.toInt()) : std::nullopt,
                    .backdropPath = object["backdrop_path"].toString(),
                    .description = object["description"].toString(),
                    .genres = std::move(genres),
                    .runtimeMinutes = runtime.isDouble() ? std::optional(runtime.toInt()) : std::nullopt
                });
            }
            emit mediaReceived(requestId, media);
        } else {
            emit mediaLoadFailed(requestId);
        }
        reply->deleteLater();
    });
    return requestId;
}

void ApiClient::updateMediaWatchStatus(const int entityId, const int watchStatusId) {
    QNetworkRequest request(buildUrl(QString("%1/%2").arg(PATH_MEDIA).arg(entityId)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.sendCustomRequest(
        request, "PATCH", QJsonDocument(QJsonObject{{"watch_status_id", watchStatusId}}).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit mediaWatchStatusChanged();
        }
        reply->deleteLater();
    });
}

int ApiClient::importMedia(const QString &url) {
    QNetworkRequest request(buildUrl(QString(PATH_MEDIA) + "/import"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setAttribute(HANDLE_ERROR_LOCALLY, true);

    const int requestId = ++mediaRequestSequence;
    QNetworkReply *reply = manager.post(
        request, QJsonDocument(QJsonObject{{"url", url}}).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId] {
        if (requestSucceeded(reply)) {
            emit mediaImported(requestId);
        } else {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QString message = "Das Medium konnte nicht importiert werden.";
            if (status == 400) {
                message = "Die TMDB-Adresse ist ungültig.";
            } else if (status == 404) {
                message = "Das Medium wurde bei TMDB nicht gefunden.";
            } else if (status == 502) {
                message = "Metadaten oder Bilder konnten bei TMDB nicht geladen werden.";
            }
            emit mediaImportFailed(requestId, message);
        }
        reply->deleteLater();
    });
    return requestId;
}
