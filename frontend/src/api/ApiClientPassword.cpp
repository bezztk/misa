#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

static QByteArray parseBytes(const QJsonValue &value) {
    QByteArray bytes;
    for (const QJsonValue &byte : value.toArray()) {
        bytes.append(static_cast<char>(byte.toInt()));
    }
    return bytes;
}

static QJsonArray serializeBytes(const QByteArray &bytes) {
    QJsonArray values;
    for (const char byte : bytes) {
        values.append(static_cast<unsigned char>(byte));
    }
    return values;
}

static QJsonValue serializeOptionalId(const std::optional<int> id) {
    return id.has_value() ? QJsonValue(*id) : QJsonValue(QJsonValue::Null);
}

static QJsonObject serializePasswordEntry(
    const std::optional<int> categoryId,
    const QByteArray &ciphertext,
    const QByteArray &salt,
    const QByteArray &nonce
) {
    return {
        {"category_id", serializeOptionalId(categoryId)},
        {"ciphertext", serializeBytes(ciphertext)},
        {"salt", serializeBytes(salt)},
        {"nonce", serializeBytes(nonce)}
    };
}

void ApiClient::getPasswordEntries() {
    QNetworkReply *reply = manager.get(QNetworkRequest(buildUrl(PATH_PASSWORD_ENTRIES)));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (!readJson(reply, "Passworteinträge laden", response) || !response.isArray()) {
            reply->deleteLater();
            return;
        }

        std::vector<PasswordEntry> entries;
        for (const QJsonValue &value : response.array()) {
            const QJsonObject object = value.toObject();
            PasswordEntry entry{
                .id = object["id"].toInt(),
                .ciphertext = parseBytes(object["ciphertext"]),
                .salt = parseBytes(object["salt"]),
                .nonce = parseBytes(object["nonce"])
            };
            if (!object["category_id"].isNull()) {
                entry.categoryId = object["category_id"].toInt();
            }
            entries.push_back(std::move(entry));
        }
        emit passwordEntriesReceived(entries);
        reply->deleteLater();
    });
}

void ApiClient::createPasswordEntry(
    const std::optional<int> categoryId,
    const QByteArray &ciphertext,
    const QByteArray &salt,
    const QByteArray &nonce
) {
    QNetworkRequest request(buildUrl(PATH_PASSWORD_ENTRIES));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.post(
        request,
        QJsonDocument(serializePasswordEntry(categoryId, ciphertext, salt, nonce)).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit passwordEntriesChanged();
        }
        reply->deleteLater();
    });
}

void ApiClient::updatePasswordEntry(
    const int id,
    const std::optional<int> categoryId,
    const QByteArray &ciphertext,
    const QByteArray &salt,
    const QByteArray &nonce
) {
    QNetworkRequest request(buildUrl(
        PATH_PASSWORD_ENTRIES + QString("/") + QString::number(id)
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PUT",
        QJsonDocument(serializePasswordEntry(categoryId, ciphertext, salt, nonce)).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit passwordEntriesChanged();
        }
        reply->deleteLater();
    });
}

void ApiClient::deletePasswordEntry(const int id) {
    QNetworkReply *reply = manager.deleteResource(QNetworkRequest(buildUrl(
        PATH_PASSWORD_ENTRIES + QString("/") + QString::number(id)
    )));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit passwordEntriesChanged();
        }
        reply->deleteLater();
    });
}

void ApiClient::getPasswordCategories() {
    QNetworkReply *reply = manager.get(QNetworkRequest(buildUrl(PATH_PASSWORD_CATEGORIES)));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (!readJson(reply, "Passwortkategorien laden", response) || !response.isArray()) {
            reply->deleteLater();
            return;
        }

        std::vector<PasswordCategory> categories;
        for (const QJsonValue &value : response.array()) {
            const QJsonObject object = value.toObject();
            categories.push_back({
                .id = object["id"].toInt(),
                .name = object["name"].toString()
            });
        }
        emit passwordCategoriesReceived(categories);
        reply->deleteLater();
    });
}

void ApiClient::createPasswordCategory(const QString &name) {
    QNetworkRequest request(buildUrl(PATH_PASSWORD_CATEGORIES));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.post(
        request,
        QJsonDocument(QJsonObject{{"name", name}}).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit passwordCategoriesChanged();
        }
        reply->deleteLater();
    });
}

void ApiClient::updatePasswordCategory(const int id, const QString &name) {
    QNetworkRequest request(buildUrl(
        PATH_PASSWORD_CATEGORIES + QString("/") + QString::number(id)
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PUT",
        QJsonDocument(QJsonObject{{"name", name}}).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit passwordCategoriesChanged();
        }
        reply->deleteLater();
    });
}

void ApiClient::deletePasswordCategory(const int id) {
    QNetworkReply *reply = manager.deleteResource(QNetworkRequest(buildUrl(
        PATH_PASSWORD_CATEGORIES + QString("/") + QString::number(id)
    )));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit passwordCategoriesChanged();
        }
        reply->deleteLater();
    });
}
