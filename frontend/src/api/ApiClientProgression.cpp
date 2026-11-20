#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

static Skill parseSkill(const QJsonObject &object) {
    Skill skill{
        .id = object["id"].toInt(),
        .domainNodeId = object["domain_node_id"].toInt(),
        .name = object["name"].toString()
    };
    skill.comments = parseComments(object["comments"].toArray());

    if (!object["parent_skill_id"].isNull()) {
        skill.parentSkillId = object["parent_skill_id"].toInt();
    }
    if (!object["psychomotor_level_id"].isNull()) {
        skill.psychomotorLevelId = object["psychomotor_level_id"].toInt();
    }

    return skill;
}

void ApiClient::getNodeSkills(const int nodeId) {
    const QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/skills"
    ));
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        QJsonDocument response;
        if (!readJson(reply, "Skills laden", response) || !response.isArray()) {
            if (!response.isNull() && !response.isArray()) {
                emit requestFailed("Skills laden: Unerwartetes Datenformat");
            }
            reply->deleteLater();
            return;
        }
        std::vector<Skill> skills;
        for (const QJsonValue &value : response.array()) {
            skills.push_back(parseSkill(value.toObject()));
        }
        emit nodeSkillsReceived(nodeId, skills);
        reply->deleteLater();
    });
}

void ApiClient::createSkill(
    const int nodeId,
    const std::optional<int> parentSkillId,
    const QString &name,
    const std::optional<int> psychomotorLevelId
) {
    QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/skills"
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject json;
    json["parent_skill_id"] = parentSkillId.has_value()
        ? QJsonValue(*parentSkillId) : QJsonValue(QJsonValue::Null);
    json["name"] = name;
    json["psychomotor_level_id"] = psychomotorLevelId.has_value()
        ? QJsonValue(*psychomotorLevelId) : QJsonValue(QJsonValue::Null);
    QNetworkReply *reply = manager.post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (requestSucceeded(reply)) {
            emit nodeSkillsChanged(nodeId);
            emit skillCreated();
        }
        reply->deleteLater();
    });
}

void ApiClient::updateSkillField(
    const int nodeId,
    const int id,
    const SkillField field,
    const QJsonValue &value
) {
    QNetworkRequest request(buildUrl(
        "/skills/" + QString::number(id)
        + "/fields/" + QString::number(static_cast<int>(field))
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject json;
    json["value"] = value;
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(json).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (requestSucceeded(reply)) {
            emit nodeSkillsChanged(nodeId);
            emit skillUpdated();
        }
        reply->deleteLater();
    });
}

static std::optional<int> optionalJsonInt(
    const QJsonObject &object,
    const QString &name
) {
    return object[name].isNull()
        ? std::nullopt
        : std::optional<int>(object[name].toInt());
}

static Pursuit parsePursuit(const QJsonObject &object) {
    Pursuit pursuit{
        .id = object["id"].toInt(),
        .domainNodeId = object["domain_node_id"].toInt(),
        .name = object["name"].toString()
    };
    pursuit.comments = parseComments(object["comments"].toArray());

    for (const QJsonValue &value : object["requirements"].toArray()) {
        const QJsonObject requirement = value.toObject();
        pursuit.requirements.push_back({
            .id = requirement["id"].toInt(),
            .requirementTypeId = requirement["requirement_type_id"].toInt(),
            .skillId = optionalJsonInt(requirement, "skill_id"),
            .knowledgeLevelId = optionalJsonInt(requirement, "knowledge_level_id"),
            .psychomotorLevelId = optionalJsonInt(requirement, "psychomotor_level_id"),
            .completenessLevelId = optionalJsonInt(requirement, "completeness_level_id"),
            .flashcardStatusId = optionalJsonInt(requirement, "flashcard_status_id"),
            .verificationStatusId = optionalJsonInt(requirement, "verification_status_id"),
            .verificationLevelId = optionalJsonInt(requirement, "verification_level_id")
        });
    }
    return pursuit;
}

void ApiClient::getNodePursuits(const int nodeId) {
    const QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/pursuits"
    ));
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        QJsonDocument response;
        if (!readJson(reply, "Vorhaben laden", response) || !response.isArray()) {
            if (!response.isNull() && !response.isArray()) {
                emit requestFailed("Vorhaben laden: Unerwartetes Datenformat");
            }
            reply->deleteLater();
            return;
        }
        std::vector<Pursuit> pursuits;
        for (const QJsonValue &value : response.array()) {
            pursuits.push_back(parsePursuit(value.toObject()));
        }
        emit nodePursuitsReceived(nodeId, pursuits);
        reply->deleteLater();
    });
}

void ApiClient::createPursuit(const int nodeId, const QString &name) {
    QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/pursuits"
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.post(
        request,
        QJsonDocument(QJsonObject{{"name", name}}).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (requestSucceeded(reply)) {
            emit nodePursuitsChanged(nodeId);
            emit pursuitCreated();
        }
        reply->deleteLater();
    });
}

void ApiClient::updatePursuitRequirements(
    const int nodeId,
    const int pursuitId,
    const UpdatePursuitRequirements &requirements
) {
    QNetworkRequest request(buildUrl(
        "/pursuits/" + QString::number(pursuitId) + "/requirements"
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject json;
    json["knowledge_level_id"] = requirements.knowledgeLevelId.has_value()
        ? QJsonValue(*requirements.knowledgeLevelId) : QJsonValue(QJsonValue::Null);
    json["completeness_level_id"] = requirements.completenessLevelId.has_value()
        ? QJsonValue(*requirements.completenessLevelId) : QJsonValue(QJsonValue::Null);
    json["flashcard_status_id"] = requirements.flashcardStatusId.has_value()
        ? QJsonValue(*requirements.flashcardStatusId) : QJsonValue(QJsonValue::Null);
    json["verification_status_id"] = requirements.verificationStatusId.has_value()
        ? QJsonValue(*requirements.verificationStatusId) : QJsonValue(QJsonValue::Null);
    json["verification_level_id"] = requirements.verificationLevelId.has_value()
        ? QJsonValue(*requirements.verificationLevelId) : QJsonValue(QJsonValue::Null);
    QJsonArray skills;
    for (const PursuitSkillRequirement &skill : requirements.skills) {
        skills.append(QJsonObject{
            {"skill_id", skill.skillId},
            {"psychomotor_level_id", skill.psychomotorLevelId}
        });
    }
    json["skills"] = skills;
    QNetworkReply *reply = manager.put(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (requestSucceeded(reply)) {
            emit nodePursuitsChanged(nodeId);
            emit pursuitUpdated();
        }
        reply->deleteLater();
    });
}

void ApiClient::getProgressions() {
    QNetworkReply *reply = manager.get(QNetworkRequest(buildUrl("/progressions")));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (readJson(reply, "Progressionen laden", response) && response.isObject()) {
            const QJsonObject object = response.object();
            std::vector<ProgressionPursuit> pursuits;
            for (const QJsonValue &value : object["pursuits"].toArray()) {
                const QJsonObject pursuit = value.toObject();
                ProgressionPursuit item{
                    .id = pursuit["id"].toInt(),
                    .name = pursuit["name"].toString()
                };
                if (!pursuit["progression_stage_id"].isNull()) {
                    item.progressionStageId = pursuit["progression_stage_id"].toInt();
                }
                pursuits.push_back(item);
            }

            std::vector<Progression> progressions;
            for (const QJsonValue &value : object["progressions"].toArray()) {
                const QJsonObject progression = value.toObject();
                Progression item{
                    .id = progression["id"].toInt(),
                    .name = progression["name"].toString()
                };
                if (!progression["personal_relevance_id"].isNull()) {
                    item.personalRelevanceId = progression["personal_relevance_id"].toInt();
                }
                if (!progression["lasting_impact_id"].isNull()) {
                    item.lastingImpactId = progression["lasting_impact_id"].toInt();
                }
                for (const QJsonValue &stageValue : progression["stages"].toArray()) {
                    const QJsonObject stage = stageValue.toObject();
                    ProgressionStage stageItem{
                        .id = stage["id"].toInt(),
                        .stage = stage["stage"].toInt(),
                        .name = stage["name"].toString()
                    };
                    for (const QJsonValue &pursuitValue : stage["pursuits"].toArray()) {
                        const QJsonObject pursuit = pursuitValue.toObject();
                        stageItem.pursuits.push_back({
                            .id = pursuit["id"].toInt(),
                            .name = pursuit["name"].toString(),
                            .progressionStageId = stageItem.id
                        });
                    }
                    item.stages.push_back(stageItem);
                }
                progressions.push_back(item);
            }
            emit progressionsReceived(progressions, pursuits);
        } else if (!response.isNull() && !response.isObject()) {
            emit requestFailed("Progressionen laden: Unerwartetes Datenformat");
        }
        reply->deleteLater();
    });
}

void ApiClient::getAvailableProgressionPursuits(const int stageId) {
    QNetworkReply *reply = manager.get(QNetworkRequest(buildUrl("/pursuits/available")));
    connect(reply, &QNetworkReply::finished, this, [this, reply, stageId] {
        QJsonDocument response;
        if (readJson(reply, "Verfügbare Vorhaben laden", response) && response.isArray()) {
            std::vector<ProgressionPursuit> pursuits;
            for (const QJsonValue &value : response.array()) {
                const QJsonObject pursuit = value.toObject();
                pursuits.push_back({
                    .id = pursuit["id"].toInt(),
                    .name = pursuit["name"].toString()
                });
            }
            emit availableProgressionPursuitsReceived(stageId, pursuits);
        } else if (!response.isNull() && !response.isArray()) {
            emit requestFailed("Verfügbare Vorhaben laden: Unerwartetes Datenformat");
        }
        reply->deleteLater();
    });
}

void ApiClient::createProgression(const QString &name) {
    QNetworkRequest request(buildUrl("/progressions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.post(
        request,
        QJsonDocument(QJsonObject{{"name", name}}).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit progressionsChanged();
            emit progressionCreated();
        }
        reply->deleteLater();
    });
}

void ApiClient::updateProgressionField(
    const int id,
    const ProgressionField field,
    const QJsonValue &value
) {
    QNetworkRequest request(buildUrl(
        "/progressions/" + QString::number(id)
        + "/fields/" + QString::number(static_cast<int>(field))
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject json{{"value", value}};
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(json).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit progressionsChanged();
            emit progressionUpdated();
        }
        reply->deleteLater();
    });
}

void ApiClient::createProgressionStage(const int progressionId, const QString &name) {
    QNetworkRequest request(buildUrl(
        "/progressions/" + QString::number(progressionId) + "/stages"
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.post(
        request,
        QJsonDocument(QJsonObject{{"name", name}}).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit progressionsChanged();
            emit progressionStageCreated();
        }
        reply->deleteLater();
    });
}

void ApiClient::assignPursuitToStage(const int stageId, const int pursuitId) {
    QNetworkRequest request(buildUrl(
        "/progression-stages/" + QString::number(stageId) + "/pursuits"
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.post(
        request,
        QJsonDocument(QJsonObject{{"pursuit_id", pursuitId}}).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit progressionsChanged();
            emit pursuitAssigned();
        }
        reply->deleteLater();
    });
}

