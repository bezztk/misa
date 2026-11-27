#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

static QJsonObject contextJson(const QString &title, const QByteArray &icon, const QColor &color) {
    QJsonArray iconBytes;
    for (const unsigned char byte : icon) {
        iconBytes.append(byte);
    }
    return {
        {"title", title},
        {"icon", icon.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(iconBytes)},
        {"color", color.isValid() ? QJsonValue(color.name()) : QJsonValue(QJsonValue::Null)}
    };
}

void ApiClient::getContexts() {
    auto *reply = manager.get(QNetworkRequest(buildUrl(PATH_CONTEXTS)));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (readJson(reply, "Kontexte laden", response) && response.isArray()) {
            std::vector<Context> contexts;
            for (const QJsonValue &value : response.array()) {
                const QJsonObject context = value.toObject();
                const QJsonArray bytes = context["icon"].toArray();
                QByteArray icon;
                icon.reserve(bytes.size());
                for (const QJsonValue &byte : bytes) {
                    icon.append(static_cast<char>(byte.toInt()));
                }
                contexts.push_back({
                    .id = context["id"].toInt(),
                    .title = context["title"].toString(),
                    .icon = std::move(icon),
                    .color = QColor(context["color"].toString()),
                    .entityCount = context["entity_count"].toInteger()
                });
            }
            emit contextsReceived(contexts);
        }
        reply->deleteLater();
    });
}

void ApiClient::createContext(const QString &title, const QByteArray &icon, const QColor &color) {
    sendChange(PATH_CONTEXTS, "POST", contextJson(title, icon, color), &ApiClient::contextsChanged);
}

void ApiClient::updateContext(
    const int id,
    const QString &title,
    const QByteArray &icon,
    const QColor &color
) {
    sendChange(
        QString("%1/%2").arg(PATH_CONTEXTS).arg(id), "PUT", contextJson(title, icon, color),
        &ApiClient::contextsChanged
    );
}

void ApiClient::deleteContext(const int id) {
    sendChange(
        QString("%1/%2").arg(PATH_CONTEXTS).arg(id), "DELETE", {}, &ApiClient::contextsChanged
    );
}
