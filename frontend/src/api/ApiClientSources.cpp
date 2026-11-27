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
                    .favicon = std::move(favicon)
                });
            }
            emit sourcesReceived(sources);
        }
        reply->deleteLater();
    });
}

void ApiClient::createSource(const QString &url) {
    sendChange(PATH_SOURCES, "POST", {{"url", url}}, &ApiClient::sourcesChanged);
}

void ApiClient::updateSourceTitle(const QString &id, const QString &title) {
    sendChange(
        QString("%1/%2/title").arg(PATH_SOURCES, id), "PATCH",
        {{"title", title}}, &ApiClient::sourcesChanged
    );
}

void ApiClient::deleteSource(const QString &id) {
    sendChange(
        QString("%1/%2").arg(PATH_SOURCES, id), "DELETE", {}, &ApiClient::sourcesChanged
    );
}
