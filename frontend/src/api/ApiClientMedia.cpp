#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <array>
#include <utility>

namespace {
constexpr std::array<const char *, 4> MEDIA_RATING_KEYS{
    "disappointing", "average", "worth_watching", "outstanding"
};

std::optional<MediaRating> parseMediaRating(const QString &key) {
    for (std::size_t index = 0; index < MEDIA_RATING_KEYS.size(); ++index) {
        if (key == QString::fromLatin1(MEDIA_RATING_KEYS[index])) {
            return static_cast<MediaRating>(index + 1);
        }
    }
    return std::nullopt;
}

QStringList parseMediaStrings(const QJsonValue &value) {
    QStringList strings;
    for (const QJsonValue &item : value.toArray()) {
        strings.append(item.toString());
    }
    return strings;
}

MediaEpisodeProgress parseMediaProgress(const QJsonObject &object) {
    return {
        .episodeCount = object["episode_count"].toInt(),
        .completedEpisodeCount = object["completed_episode_count"].toInt(),
        .percentage = object["percentage"].toDouble()
    };
}
}

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
                const QJsonValue runtime = object["runtime_minutes"];
                const QJsonValue voteAverage = object["tmdb_vote_average"];
                const QJsonValue voteCount = object["tmdb_vote_count"];
                media.push_back({
                    .entityId = object["entity_id"].toInt(),
                    .tmdbId = object["tmdb_id"].toInt(),
                    .mediaType = object["media_type"].toString(),
                    .title = object["title"].toString(),
                    .originalTitle = object["original_title"].toString(),
                    .releaseDate = QDate::fromString(object["release_date"].toString(), Qt::ISODate),
                    .posterPath = object["poster_path"].toString(),
                    .watchStatusId = object["watch_status_id"].toInt(),
                    .personalRating = parseMediaRating(object["personal_rating"].toString()),
                    .backdropPath = object["backdrop_path"].toString(),
                    .description = object["description"].toString(),
                    .genres = parseMediaStrings(object["genres"]),
                    .runtimeMinutes = runtime.isDouble() ? std::optional(runtime.toInt()) : std::nullopt,
                    .tmdbVoteAverage = voteAverage.isDouble() ? std::optional(voteAverage.toDouble()) : std::nullopt,
                    .tmdbVoteCount = voteCount.isDouble() ? std::optional(voteCount.toInt()) : std::nullopt,
                    .productionStatus = object["production_status"].toString(),
                    .keywords = parseMediaStrings(object["keywords"]),
                    .posterUrl = object["poster_url"].toString(),
                    .backdropUrl = object["backdrop_url"].toString(),
                    .trailerUrl = object["trailer_url"].toString(),
                    .additionalBackdropPaths = parseMediaStrings(object["additional_backdrop_paths"]),
                    .seasonCount = object["season_count"].toInt(),
                    .episodeCount = object["episode_count"].toInt(),
                    .watchCount = object["watch_count"].toInt(),
                    .completedAt = QDateTime::fromString(object["completed_at"].toString(), Qt::ISODate),
                    .lastSyncedAt = QDateTime::fromString(object["last_synced_at"].toString(), Qt::ISODate),
                    .personalNotes = object["personal_notes"].toString()
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
    updateMediaPersonalData(entityId, {{"watch_status_id", watchStatusId}});
}

int ApiClient::getMediaJson(
    const QString &path, const QString &operation,
    std::function<bool(int, const QJsonDocument &)> received
) {
    const int requestId = ++mediaRequestSequence;
    QNetworkRequest request(buildUrl(path));
    request.setAttribute(HANDLE_ERROR_LOCALLY, true);
    QNetworkReply *reply = manager.get(request);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, requestId, operation, received = std::move(received)] {
                QJsonDocument response;
                if (!readJson(reply, operation, response) || !received(requestId, response)) {
                    emit mediaDetailsLoadFailed(requestId);
                }
                reply->deleteLater();
            });
    return requestId;
}

int ApiClient::getMediaSeasons(const int entityId) {
    return getMediaJson(QString("%1/%2/seasons").arg(PATH_MEDIA).arg(entityId), "Staffeln laden",
        [this](const int requestId, const QJsonDocument &response) {
            if (!response.isArray()) {
                return false;
            }
            std::vector<MediaSeason> seasons;
            for (const QJsonValue &value : response.array()) {
                const QJsonObject object = value.toObject();
                seasons.push_back({
                    .id = object["id"].toInt(),
                    .number = object["season_number"].toInt(),
                    .title = object["title"].toString(),
                    .description = object["description"].toString(),
                    .posterPath = object["poster_path"].toString(),
                    .releaseDate = QDate::fromString(object["release_date"].toString(), Qt::ISODate),
                    .episodeCount = object["episode_count"].toInt(),
                    .completedEpisodeCount = object["completed_episode_count"].toInt()
                });
            }
            emit mediaSeasonsReceived(requestId, seasons);
            return true;
        });
}

int ApiClient::getMediaEpisodes(const int entityId, const int seasonId) {
    return getMediaJson(QString("%1/%2/seasons/%3/episodes").arg(PATH_MEDIA).arg(entityId).arg(seasonId),
        "Episoden laden", [this](const int requestId, const QJsonDocument &response) {
            if (!response.isArray()) {
                return false;
            }
            std::vector<MediaEpisode> episodes;
            for (const QJsonValue &value : response.array()) {
                const QJsonObject object = value.toObject();
                const QJsonValue runtime = object["runtime_minutes"];
                episodes.push_back({
                    .id = object["id"].toInt(),
                    .number = object["episode_number"].toInt(),
                    .title = object["title"].toString(),
                    .description = object["description"].toString(),
                    .stillPath = object["still_path"].toString(),
                    .runtimeMinutes = runtime.isDouble() ? std::optional(runtime.toInt()) : std::nullopt,
                    .releaseDate = QDate::fromString(object["release_date"].toString(), Qt::ISODate),
                    .watchStatusId = object["watch_status_id"].toInt(),
                    .watchedAt = QDateTime::fromString(object["watched_at"].toString(), Qt::ISODate)
                });
            }
            emit mediaEpisodesReceived(requestId, episodes);
            return true;
        });
}

int ApiClient::getMediaSeriesProgress(const int entityId) {
    return getMediaJson(QString("%1/%2/progress").arg(PATH_MEDIA).arg(entityId), "Serienfortschritt laden",
        [this](const int requestId, const QJsonDocument &response) {
            const QJsonObject object = response.object();
            if (!object["regular"].isObject() || !object["specials"].isObject()) {
                return false;
            }
            emit mediaSeriesProgressReceived(requestId, {
                .regular = parseMediaProgress(object["regular"].toObject()),
                .specials = parseMediaProgress(object["specials"].toObject())
            });
            return true;
        });
}

int ApiClient::patchMediaJson(
    const QString &path, const QJsonObject &fields, std::function<void(int, bool)> finished
) {
    const int requestId = ++mediaRequestSequence;
    QNetworkRequest request(buildUrl(path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.sendCustomRequest(request, "PATCH", QJsonDocument(fields).toJson());
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, requestId, finished = std::move(finished)] {
                finished(requestId, requestSucceeded(reply));
                reply->deleteLater();
            });
    return requestId;
}

int ApiClient::updateMediaPersonalData(const int entityId, const QJsonObject &fields) {
    return patchMediaJson(QString("%1/%2").arg(PATH_MEDIA).arg(entityId), fields,
        [this](const int requestId, const bool successful) {
            emit mediaPersonalDataUpdateFinished(requestId, successful);
        });
}

int ApiClient::updateMediaEpisodeStatus(
    const int entityId, const int seasonId, const int episodeId, const int watchStatusId
) {
    return patchMediaJson(QString("%1/%2/seasons/%3/episodes/%4")
        .arg(PATH_MEDIA).arg(entityId).arg(seasonId).arg(episodeId), {{"watch_status_id", watchStatusId}},
        [this, entityId, seasonId](const int requestId, const bool successful) {
            emit mediaEpisodeStatusUpdateFinished(requestId, entityId, seasonId, successful);
        });
}

void ApiClient::updateMediaRating(const int entityId, const std::optional<MediaRating> rating) {
    const QJsonValue value = rating
        ? QJsonValue(QString::fromLatin1(MEDIA_RATING_KEYS[static_cast<std::size_t>(*rating) - 1]))
        : QJsonValue(QJsonValue::Null);
    patchMediaJson(QString("%1/%2").arg(PATH_MEDIA).arg(entityId), {{"personal_rating", value}},
        [this, entityId, rating](int, const bool successful) {
            emit mediaRatingUpdateFinished(entityId, rating, successful);
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
