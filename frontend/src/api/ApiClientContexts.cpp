#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

void ApiClient::getContexts() {
    auto *reply = manager.get(QNetworkRequest(buildUrl(PATH_CONTEXTS)));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (readJson(reply, "Kontexte laden", response) && response.isArray()) {
            std::vector<Context> contexts;
            for (const QJsonValue &value : response.array()) {
                const QJsonObject context = value.toObject();
                contexts.push_back({
                    .id = context["id"].toInt(),
                    .title = context["title"].toString()
                });
            }
            emit contextsReceived(contexts);
        }
        reply->deleteLater();
    });
}

void ApiClient::createContext(const QString &title) {
    sendChange(PATH_CONTEXTS, "POST", {{"title", title}}, &ApiClient::contextsChanged);
}

void ApiClient::renameContext(const int id, const QString &title) {
    sendChange(
        QString("%1/%2").arg(PATH_CONTEXTS).arg(id), "PUT", {{"title", title}},
        &ApiClient::contextsChanged
    );
}

void ApiClient::deleteContext(const int id) {
    sendChange(
        QString("%1/%2").arg(PATH_CONTEXTS).arg(id), "DELETE", {}, &ApiClient::contextsChanged
    );
}
