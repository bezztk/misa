#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

void ApiClient::getRssFeeds() {
    QNetworkReply *reply = manager.get(QNetworkRequest(buildUrl(PATH_RSS_FEEDS)));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (!readJson(reply, "RSS-Abonnements laden", response) || !response.isArray()) {
            reply->deleteLater();
            return;
        }

        std::vector<RssFeed> feeds;
        for (const QJsonValue &value : response.array()) {
            const QJsonObject object = value.toObject();
            RssFeed feed{
                .id = object["id"].toInt(),
                .url = object["url"].toString(),
                .title = object["title"].toString(),
                .enabled = object["enabled"].toBool(),
                .isHidden = object["is_hidden"].toBool()
            };
            if (!object["last_fetched_at"].isNull()) {
                feed.lastFetchedAt = QDateTime::fromString(
                    object["last_fetched_at"].toString(), Qt::ISODate
                );
            }
            feeds.push_back(std::move(feed));
        }
        emit rssFeedsReceived(feeds);
        reply->deleteLater();
    });
}

void ApiClient::createRssFeed(const QString &url) {
    QNetworkRequest request(buildUrl(PATH_RSS_FEEDS));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.post(
        request,
        QJsonDocument(QJsonObject{{"url", url}}).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit rssFeedCreated();
        }
        reply->deleteLater();
    });
}

void ApiClient::updateRssFeedEnabled(const int id, const bool enabled) {
    QNetworkRequest request(buildUrl(
        PATH_RSS_FEEDS + QString("/") + QString::number(id)
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(QJsonObject{{"enabled", enabled}}).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit rssFeedUpdated();
        }
        reply->deleteLater();
    });
}

void ApiClient::deleteRssFeed(const int id) {
    QNetworkReply *reply = manager.deleteResource(QNetworkRequest(buildUrl(
        PATH_RSS_FEEDS + QString("/") + QString::number(id)
    )));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit rssFeedDeleted();
        }
        reply->deleteLater();
    });
}

void ApiClient::getRssEntries() {
    QNetworkReply *reply = manager.get(QNetworkRequest(buildUrl(PATH_RSS_ENTRIES)));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (!readJson(reply, "RSS-Einträge laden", response) || !response.isArray()) {
            reply->deleteLater();
            return;
        }

        std::vector<RssEntry> entries;
        for (const QJsonValue &value : response.array()) {
            const QJsonObject object = value.toObject();
            RssEntry entry{
                .id = object["id"].toInt(),
                .feedId = object["feed_id"].toInt(),
                .feedTitle = object["feed_title"].toString(),
                .externalId = object["external_id"].toString(),
                .title = object["title"].toString(),
                .url = object["url"].toString(),
                .summary = object["summary"].toString(),
                .isRead = object["is_read"].toBool()
            };
            if (!object["published_at"].isNull()) {
                entry.publishedAt = QDateTime::fromString(
                    object["published_at"].toString(), Qt::ISODate
                );
            }
            entries.push_back(std::move(entry));
        }
        emit rssEntriesReceived(entries);
        reply->deleteLater();
    });
}

void ApiClient::updateRssEntryRead(const int id, const bool isRead) {
    QNetworkRequest request(buildUrl(
        PATH_RSS_ENTRIES + QString("/") + QString::number(id)
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(QJsonObject{{"is_read", isRead}}).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit rssEntryUpdated();
        }
        reply->deleteLater();
    });
}
