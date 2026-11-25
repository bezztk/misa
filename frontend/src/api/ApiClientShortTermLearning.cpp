#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

namespace {

constexpr auto setsPath = "/short-term-learning/sets";

QString setPath(const int id) {
    return QString("%1/%2").arg(setsPath).arg(id);
}

QString cardPath(const int id) {
    return QString("/short-term-learning/cards/%1").arg(id);
}

} // namespace

void ApiClient::getShortTermLearningSets(
    QObject &context,
    std::function<void(std::optional<std::vector<ShortTermLearningSet>>)> received
) {
    auto *reply = manager.get(QNetworkRequest(buildUrl(setsPath)));
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    connect(reply, &QNetworkReply::finished, &context, [this, reply, received = std::move(received)] {
        QJsonDocument response;
        if (!readJson(reply, "Lernsätze laden", response)) {
            received(std::nullopt);
            return;
        }
        if (!response.isArray()) {
            emit requestFailed("Lernsätze laden: Ungültige Serverantwort");
            received(std::nullopt);
            return;
        }
        std::vector<ShortTermLearningSet> sets;
        for (const QJsonValue &value : response.array()) {
            const auto object = value.toObject();
            sets.push_back({
                .id = object["id"].toInt(),
                .title = object["title"].toString(),
                .bidirectional = object["bidirectional"].toBool(),
                .cardCount = object["card_count"].toInteger(),
                .newCount = object["new_count"].toInteger(),
                .learningCount = object["learning_count"].toInteger(),
                .completedCount = object["completed_count"].toInteger(),
            });
        }
        received(std::move(sets));
    });
}

void ApiClient::createShortTermLearningSet(
    const QString &title, const bool bidirectional,
    QObject &context, std::function<void(bool)> finished
) {
    sendShortTermLearningChange(setsPath, "POST",
        QJsonDocument(QJsonObject{{"title", title}, {"bidirectional", bidirectional}}),
        context, std::move(finished));
}

void ApiClient::deleteShortTermLearningSet(
    const int id, QObject &context, std::function<void(bool)> finished
) {
    sendShortTermLearningChange(setPath(id), "DELETE", {}, context, std::move(finished));
}

void ApiClient::resetShortTermLearningSet(
    const int id, QObject &context, std::function<void(bool)> finished
) {
    sendShortTermLearningChange(setPath(id) + "/reset", "POST", {}, context, std::move(finished));
}

void ApiClient::getShortTermLearningCards(
    const int setId, QObject &context,
    std::function<void(std::optional<std::vector<ShortTermLearningCard>>)> received
) {
    auto *reply = manager.get(QNetworkRequest(buildUrl(setPath(setId) + "/cards")));
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    connect(reply, &QNetworkReply::finished, &context, [this, reply, received = std::move(received)] {
        QJsonDocument response;
        if (!readJson(reply, "Lernkarten laden", response)) {
            received(std::nullopt);
            return;
        }
        if (!response.isArray()) {
            emit requestFailed("Lernkarten laden: Ungültige Serverantwort");
            received(std::nullopt);
            return;
        }
        std::vector<ShortTermLearningCard> cards;
        for (const QJsonValue &value : response.array()) {
            const auto object = value.toObject();
            const auto reverseState = object["back_to_front_state_id"];
            cards.push_back({
                .id = object["id"].toInt(),
                .front = object["front"].toString(),
                .hint = object["hint"].toString(),
                .back = object["back"].toString(),
                .frontToBackState = static_cast<ShortTermLearningState>(
                    object["front_to_back_state_id"].toInt()
                ),
                .backToFrontState = reverseState.isNull() ? std::nullopt
                    : std::optional(static_cast<ShortTermLearningState>(reverseState.toInt())),
            });
        }
        received(std::move(cards));
    });
}

void ApiClient::importShortTermLearningCards(
    const int setId, const QJsonArray &cards, QObject &context, std::function<void(bool)> finished
) {
    sendShortTermLearningChange(setPath(setId) + "/cards", "POST", QJsonDocument(cards),
        context, std::move(finished));
}

void ApiClient::updateShortTermLearningCard(
    const ShortTermLearningCard &card, QObject &context, std::function<void(bool)> finished
) {
    sendShortTermLearningChange(cardPath(card.id), "PATCH", QJsonDocument(QJsonObject{
        {"front", card.front},
        {"hint", card.hint.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(card.hint)},
        {"back", card.back},
        {"front_to_back_state_id", static_cast<int>(card.frontToBackState)},
        {"back_to_front_state_id", card.backToFrontState
            ? QJsonValue(static_cast<int>(*card.backToFrontState)) : QJsonValue(QJsonValue::Null)},
    }), context, std::move(finished));
}

void ApiClient::deleteShortTermLearningCard(
    const int id, QObject &context, std::function<void(bool)> finished
) {
    sendShortTermLearningChange(cardPath(id), "DELETE", {}, context, std::move(finished));
}

void ApiClient::rateShortTermLearningCard(
    const int id, const bool reverse, const ShortTermLearningState state,
    QObject &context, std::function<void(bool)> finished
) {
    sendShortTermLearningChange(cardPath(id) + "/state", "PATCH", QJsonDocument(QJsonObject{
        {"reverse", reverse},
        {"state_id", static_cast<int>(state)},
    }), context, std::move(finished));
}

void ApiClient::sendShortTermLearningChange(
    const QString &path, const QByteArray &method, const QJsonDocument &body,
    QObject &context, std::function<void(bool)> finished
) {
    QNetworkRequest request(buildUrl(path));
    QByteArray data;
    if (!body.isNull()) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        data = body.toJson(QJsonDocument::Compact);
    }
    auto *reply = manager.sendCustomRequest(request, method, data);
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    connect(reply, &QNetworkReply::finished, &context, [this, reply, finished = std::move(finished)] {
        finished(requestSucceeded(reply));
    });
}
