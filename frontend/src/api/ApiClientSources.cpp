#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

void ApiClient::getSources() {
    QNetworkReply *reply = manager.get(QNetworkRequest(buildUrl(PATH_SOURCES)));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (readJson(reply, "Quellen laden", response) && response.isArray()) {
            std::vector<Source> sources;
            for (const QJsonValue &value : response.array()) {
                const QJsonObject source = value.toObject();
                const QJsonArray bytes = source["favicon"].toArray();
                QByteArray favicon;
                favicon.reserve(bytes.size());
                for (const QJsonValue &byte : bytes) {
                    favicon.append(static_cast<char>(byte.toInt()));
                }
                sources.push_back({
                    .id = source["id"].toString(),
                    .title = source["title"].toString(),
                    .url = source["url"].toString(),
                    .resourceType = source["resource_type"].toString(),
                    .description = source["description"].toString(),
                    .resourceTypeId = source["resource_type_id"].toInt(),
                    .categoryId = source["category_id"].toInt(),
                    .favicon = std::move(favicon)
                });
            }
            emit sourcesReceived(sources);
        }
        reply->deleteLater();
    });
}

void ApiClient::createSource(const QString &url) {
    sendSourceChange(PATH_SOURCES, "POST", {{"url", url}}, &ApiClient::sourcesChanged);
}

void ApiClient::updateSourceTitle(const QString &id, const QString &title) {
    sendSourceChange(
        QString("%1/%2/title").arg(PATH_SOURCES, id), "PATCH",
        {{"title", title}}, &ApiClient::sourcesChanged
    );
}

void ApiClient::assignSourceCategory(const QString &id, const int categoryId) {
    const QJsonValue category = categoryId == 0 ? QJsonValue(QJsonValue::Null) : QJsonValue(categoryId);
    sendSourceChange(
        QString("%1/%2/category").arg(PATH_SOURCES, id), "PUT",
        {{"category_id", category}}, &ApiClient::sourcesChanged
    );
}

void ApiClient::deleteSource(const QString &id) {
    sendSourceChange(
        QString("%1/%2").arg(PATH_SOURCES, id), "DELETE", {}, &ApiClient::sourcesChanged
    );
}

void ApiClient::getSourceCategories() {
    auto *reply = manager.get(QNetworkRequest(buildUrl(PATH_SOURCE_CATEGORIES)));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (readJson(reply, "Quellenkategorien laden", response) && response.isArray()) {
            std::vector<SourceCategory> categories;
            for (const QJsonValue &value : response.array()) {
                const QJsonObject category = value.toObject();
                categories.push_back({
                    .id = category["id"].toInt(),
                    .name = category["name"].toString()
                });
            }
            emit sourceCategoriesReceived(categories);
        }
        reply->deleteLater();
    });
}

void ApiClient::createSourceCategory(const QString &name) {
    sendSourceChange(
        PATH_SOURCE_CATEGORIES, "POST", {{"name", name}}, &ApiClient::sourceCategoriesChanged
    );
}

void ApiClient::renameSourceCategory(const int id, const QString &name) {
    sendSourceChange(
        QString("%1/%2").arg(PATH_SOURCE_CATEGORIES).arg(id), "PUT",
        {{"name", name}}, &ApiClient::sourceCategoriesChanged
    );
}

void ApiClient::deleteSourceCategory(const int id) {
    sendSourceChange(
        QString("%1/%2").arg(PATH_SOURCE_CATEGORIES).arg(id), "DELETE", {},
        &ApiClient::sourceCategoriesChanged
    );
}

void ApiClient::sendSourceChange(
    const QString &path,
    const QByteArray &method,
    const QJsonObject &body,
    void (ApiClient::*changed)()
) {
    QNetworkRequest request(buildUrl(path));
    QByteArray data;
    if (!body.isEmpty()) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        data = QJsonDocument(body).toJson();
    }
    auto *reply = manager.sendCustomRequest(request, method, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply, changed] {
        if (requestSucceeded(reply)) {
            emit (this->*changed)();
        }
        reply->deleteLater();
    });
}
