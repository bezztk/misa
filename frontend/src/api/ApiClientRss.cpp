#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

static QJsonValue optionalId(const std::optional<int> id) {
    return id.has_value() ? QJsonValue(*id) : QJsonValue(QJsonValue::Null);
}

static QJsonValue optionalBytes(const std::optional<QByteArray> &bytes) {
    if (!bytes.has_value()) {
        return QJsonValue(QJsonValue::Null);
    }
    QJsonArray values;
    for (const char byte : *bytes) {
        values.append(static_cast<unsigned char>(byte));
    }
    return values;
}

static RssCategory parseRssCategory(const QJsonObject &object) {
    RssCategory category{
        .id = object["id"].toInt(),
        .name = object["name"].toString(),
        .color = object["color"].toString()
    };
    if (!object["icon"].isNull()) {
        QByteArray icon;
        for (const QJsonValue &byte : object["icon"].toArray()) {
            icon.append(static_cast<char>(byte.toInt()));
        }
        category.icon = icon;
    }
    return category;
}

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
            if (!object["category_id"].isNull()) {
                feed.categoryId = object["category_id"].toInt();
            }
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

void ApiClient::updateRssFeed(
    const int id,
    const QString &url,
    const bool enabled
) {
    QNetworkRequest request(buildUrl(
        PATH_RSS_FEEDS + QString("/") + QString::number(id)
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(QJsonObject{{"url", url}, {"enabled", enabled}}).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit rssFeedUpdated();
        }
        reply->deleteLater();
    });
}

void ApiClient::assignRssFeedCategory(
    const int id,
    const std::optional<int> categoryId
) {
    QNetworkRequest request(buildUrl(
        PATH_RSS_FEEDS + QString("/") + QString::number(id) + "/category"
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PUT",
        QJsonDocument(QJsonObject{{"category_id", optionalId(categoryId)}}).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit rssFeedUpdated();
        }
        reply->deleteLater();
    });
}

void ApiClient::refreshRssFeed(const int id) {
    QNetworkReply *reply = manager.post(
        QNetworkRequest(buildUrl(
            PATH_RSS_FEEDS + QString("/") + QString::number(id) + "/refresh"
        )),
        QByteArray()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit rssFeedsRefreshed();
        }
        reply->deleteLater();
    });
}

void ApiClient::refreshAllRssFeeds() {
    QNetworkReply *reply = manager.post(
        QNetworkRequest(buildUrl(PATH_RSS_FEEDS + QString("/refresh"))),
        QByteArray()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit rssFeedsRefreshed();
        }
        reply->deleteLater();
    });
}

void ApiClient::markRssFeedRead(const int id) {
    QNetworkReply *reply = manager.post(
        QNetworkRequest(buildUrl(
            PATH_RSS_FEEDS + QString("/") + QString::number(id) + "/read"
        )),
        QByteArray()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit rssFeedEntriesRead();
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

void ApiClient::getRssCategories() {
    QNetworkReply *reply = manager.get(QNetworkRequest(buildUrl(PATH_RSS_CATEGORIES)));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (!readJson(reply, "RSS-Kategorien laden", response) || !response.isArray()) {
            reply->deleteLater();
            return;
        }
        std::vector<RssCategory> categories;
        for (const QJsonValue &value : response.array()) {
            categories.push_back(parseRssCategory(value.toObject()));
        }
        emit rssCategoriesReceived(categories);
        reply->deleteLater();
    });
}

void ApiClient::createRssCategory(
    const QString &name,
    const std::optional<QByteArray> &icon,
    const QString &color
) {
    QNetworkRequest request(buildUrl(PATH_RSS_CATEGORIES));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.post(
        request,
        QJsonDocument(QJsonObject{
            {"name", name},
            {"icon", optionalBytes(icon)},
            {"color", color}
        }).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit rssCategoriesChanged();
        }
        reply->deleteLater();
    });
}

void ApiClient::updateRssCategory(
    const int id,
    const QString &name,
    const std::optional<QByteArray> &icon,
    const QString &color
) {
    QNetworkRequest request(buildUrl(
        PATH_RSS_CATEGORIES + QString("/") + QString::number(id)
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PUT",
        QJsonDocument(QJsonObject{
            {"name", name},
            {"icon", optionalBytes(icon)},
            {"color", color}
        }).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit rssCategoriesChanged();
        }
        reply->deleteLater();
    });
}

void ApiClient::deleteRssCategory(const int id) {
    QNetworkReply *reply = manager.deleteResource(QNetworkRequest(buildUrl(
        PATH_RSS_CATEGORIES + QString("/") + QString::number(id)
    )));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit rssCategoriesChanged();
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
