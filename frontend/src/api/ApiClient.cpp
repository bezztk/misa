#include "ApiClient.h"
#include "../models/AppState.h"
#include "../models/Task.h"
#include "../models/CalendarEntry.h"
#include <QNetworkReply>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ApiClient::ApiClient() = default;

static MetadataItem parseMetadataItem(const QJsonValue &value) {
    const QJsonObject object = value.toObject();
    MetadataItem item{
        .id = object["id"].toInt(),
        .name = object["name"].toString()
    };

    if (!object["display_name"].isNull()) {
        item.display_name = object["display_name"].toString();
    }

    return item;
}

static std::vector<MetadataItem> parseMetadataItems(const QJsonArray &array) {
    std::vector<MetadataItem> items;

    for (const QJsonValue &value : array) {
        items.push_back(parseMetadataItem(value));
    }

    return items;
}

static DomainNode parseDomainNode(const QJsonObject &object) {
    DomainNode node{
        .id = object["id"].toInt(),
        .domainId = object["domain_id"].toInt(),
        .name = object["name"].toString(),
        .hasDocument = object["has_document"].toBool(),
        .flashcardCount = object["flashcard_count"].toInt(),
        .skillCount = object["skill_count"].toInt(),
        .pursuitCount = object["pursuit_count"].toInt()
    };

    if (!object["parent_id"].isNull()) {
        node.parentId = object["parent_id"].toInt();
    }
    if (!object["cognitive_level_id"].isNull()) {
        node.cognitiveLevelId = object["cognitive_level_id"].toInt();
    }
    if (!object["completeness_level_id"].isNull()) {
        node.completenessLevelId = object["completeness_level_id"].toInt();
    }
    if (!object["flashcard_status_id"].isNull()) {
        node.flashcardStatusId = object["flashcard_status_id"].toInt();
    }
    if (!object["verification_status_id"].isNull()) {
        node.verificationStatusId = object["verification_status_id"].toInt();
    }
    if (!object["verification_level_id"].isNull()) {
        node.verificationLevelId = object["verification_level_id"].toInt();
    }

    return node;
}

static Skill parseSkill(const QJsonObject &object) {
    Skill skill{
        .id = object["id"].toInt(),
        .domainNodeId = object["domain_node_id"].toInt(),
        .name = object["name"].toString()
    };

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
        if (reply->error() == QNetworkReply::NoError) {
            std::vector<Skill> skills;
            for (const QJsonValue &value : QJsonDocument::fromJson(reply->readAll()).array()) {
                skills.push_back(parseSkill(value.toObject()));
            }
            emit nodeSkillsReceived(nodeId, skills);
        }
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
        if (reply->error() == QNetworkReply::NoError) {
            emit nodeSkillsChanged(nodeId);
        }
        reply->deleteLater();
    });
}

void ApiClient::updateSkill(
    const int nodeId,
    const int id,
    const QString &name,
    const std::optional<int> psychomotorLevelId
) {
    QNetworkRequest request(buildUrl("/skills/" + QString::number(id)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject json;
    json["name"] = name;
    json["psychomotor_level_id"] = psychomotorLevelId.has_value()
        ? QJsonValue(*psychomotorLevelId) : QJsonValue(QJsonValue::Null);
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(json).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (reply->error() == QNetworkReply::NoError) {
            emit nodeSkillsChanged(nodeId);
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

    for (const QJsonValue &value : object["requirements"].toArray()) {
        const QJsonObject requirement = value.toObject();
        pursuit.requirements.push_back({
            .id = requirement["id"].toInt(),
            .requirementTypeId = requirement["requirement_type_id"].toInt(),
            .skillId = optionalJsonInt(requirement, "skill_id"),
            .cognitiveLevelId = optionalJsonInt(requirement, "cognitive_level_id"),
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
        if (reply->error() == QNetworkReply::NoError) {
            std::vector<Pursuit> pursuits;
            for (const QJsonValue &value : QJsonDocument::fromJson(reply->readAll()).array()) {
                pursuits.push_back(parsePursuit(value.toObject()));
            }
            emit nodePursuitsReceived(nodeId, pursuits);
        }
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
        if (reply->error() == QNetworkReply::NoError) {
            emit nodePursuitsChanged(nodeId);
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
    json["cognitive_level_id"] = requirements.cognitiveLevelId.has_value()
        ? QJsonValue(*requirements.cognitiveLevelId) : QJsonValue(QJsonValue::Null);
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
        if (reply->error() == QNetworkReply::NoError) {
            emit nodePursuitsChanged(nodeId);
        }
        reply->deleteLater();
    });
}

void ApiClient::getProgressions() {
    QNetworkReply *reply = manager.get(QNetworkRequest(buildUrl("/progressions")));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonObject object = QJsonDocument::fromJson(reply->readAll()).object();
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
        }
        reply->deleteLater();
    });
}

void ApiClient::getAvailableProgressionPursuits(const int stageId) {
    QNetworkReply *reply = manager.get(QNetworkRequest(buildUrl("/pursuits/available")));
    connect(reply, &QNetworkReply::finished, this, [this, reply, stageId] {
        if (reply->error() == QNetworkReply::NoError) {
            std::vector<ProgressionPursuit> pursuits;
            for (const QJsonValue &value
                 : QJsonDocument::fromJson(reply->readAll()).array()) {
                const QJsonObject pursuit = value.toObject();
                pursuits.push_back({
                    .id = pursuit["id"].toInt(),
                    .name = pursuit["name"].toString()
                });
            }
            emit availableProgressionPursuitsReceived(stageId, pursuits);
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
        if (reply->error() == QNetworkReply::NoError) {
            emit progressionsChanged();
        }
        reply->deleteLater();
    });
}

void ApiClient::updateProgression(
    const int id,
    const QString &name,
    const std::optional<int> personalRelevanceId,
    const std::optional<int> lastingImpactId
) {
    QNetworkRequest request(buildUrl("/progressions/" + QString::number(id)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject json;
    json["name"] = name;
    json["personal_relevance_id"] = personalRelevanceId.has_value()
        ? QJsonValue(*personalRelevanceId) : QJsonValue(QJsonValue::Null);
    json["lasting_impact_id"] = lastingImpactId.has_value()
        ? QJsonValue(*lastingImpactId) : QJsonValue(QJsonValue::Null);
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(json).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError) {
            emit progressionsChanged();
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
        if (reply->error() == QNetworkReply::NoError) {
            emit progressionsChanged();
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
        if (reply->error() == QNetworkReply::NoError) {
            emit progressionsChanged();
        }
        reply->deleteLater();
    });
}

void ApiClient::getNodeFlashcards(const int nodeId) {
    const QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/flashcards"
    ));
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (reply->error() == QNetworkReply::NoError) {
            std::vector<Flashcard> flashcards;
            for (const QJsonValue &value : QJsonDocument::fromJson(reply->readAll()).array()) {
                const QJsonObject object = value.toObject();
                flashcards.push_back({
                    .id = object["id"].toInt(),
                    .front = object["front"].toString(),
                    .back = object["back"].toString()
                });
            }
            emit nodeFlashcardsReceived(nodeId, flashcards);
        }
        reply->deleteLater();
    });
}

void ApiClient::createFlashcard(
    const int nodeId,
    const QString &front,
    const QString &back
) {
    QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/flashcards"
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject json{{"front", front}, {"back", back}};
    QNetworkReply *reply = manager.post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (reply->error() == QNetworkReply::NoError) {
            emit nodeFlashcardsChanged(nodeId);
        }
        reply->deleteLater();
    });
}

void ApiClient::updateFlashcard(
    const int nodeId,
    const int id,
    const QString &front,
    const QString &back
) {
    QNetworkRequest request(buildUrl("/flashcards/" + QString::number(id)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject json{{"front", front}, {"back", back}};
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(json).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (reply->error() == QNetworkReply::NoError) {
            emit nodeFlashcardsChanged(nodeId);
        }
        reply->deleteLater();
    });
}

void ApiClient::deleteFlashcard(const int nodeId, const int id) {
    const QNetworkRequest request(buildUrl("/flashcards/" + QString::number(id)));
    QNetworkReply *reply = manager.deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (reply->error() == QNetworkReply::NoError) {
            emit nodeFlashcardsChanged(nodeId);
        }
        reply->deleteLater();
    });
}

static Document parseDocument(const QJsonObject &object) {
    Document document{
        .domainNodeId = object["domain_node_id"].toInt(),
        .content = object["content"].toString()
    };

    if (!object["completeness_level_id"].isNull()) {
        document.completenessLevelId = object["completeness_level_id"].toInt();
    }
    if (!object["flashcard_status_id"].isNull()) {
        document.flashcardStatusId = object["flashcard_status_id"].toInt();
    }
    if (!object["verification_status_id"].isNull()) {
        document.verificationStatusId = object["verification_status_id"].toInt();
    }
    if (!object["verification_level_id"].isNull()) {
        document.verificationLevelId = object["verification_level_id"].toInt();
    }

    return document;
}

QUrl ApiClient::buildUrl(const QString &path) {
    QUrl url;

    url.setScheme(SCHEME);
    url.setHost(HOST);
    url.setPort(PORT);
    url.setPath(QString(BASE_PATH) + path);

    return url;
}

void ApiClient::getMetadata() {
    const QNetworkRequest networkRequest(buildUrl(PATH_METADATA));
    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QJsonObject object = QJsonDocument::fromJson(reply->readAll()).object();
        Metadata metadata;

        metadata.personal_relevance_levels = parseMetadataItems(object["personal_relevance_levels"].toArray());
        metadata.lasting_impacts = parseMetadataItems(object["lasting_impacts"].toArray());
        metadata.cognitive_levels = parseMetadataItems(object["cognitive_levels"].toArray());
        metadata.psychomotor_levels = parseMetadataItems(object["psychomotor_levels"].toArray());
        metadata.document_completeness_levels = parseMetadataItems(object["document_completeness_levels"].toArray());
        metadata.document_flashcard_statuses = parseMetadataItems(object["document_flashcard_statuses"].toArray());
        metadata.document_verification_statuses = parseMetadataItems(object["document_verification_statuses"].toArray());
        metadata.document_verification_levels = parseMetadataItems(object["document_verification_levels"].toArray());
        metadata.flashcard_review_states = parseMetadataItems(object["flashcard_review_states"].toArray());
        metadata.pursuit_requirement_types = parseMetadataItems(object["pursuit_requirement_types"].toArray());
        metadata.systemtask_groups = parseMetadataItems(object["systemtask_groups"].toArray());
        metadata.systemtask_states = parseMetadataItems(object["systemtask_states"].toArray());
        metadata.systemtask_priorities = parseMetadataItems(object["systemtask_priorities"].toArray());
        metadata.systemtask_routine_execution_states = parseMetadataItems(object["systemtask_routine_execution_states"].toArray());
        metadata.systemtask_session_states = parseMetadataItems(object["systemtask_session_states"].toArray());

        for (const QJsonValue &value : object["flashcard_retention_classes"].toArray()) {
            const QJsonObject level = value.toObject();
            FlashcardRetentionClass retentionClass{
                .id = level["id"].toInt(),
                .name = level["name"].toString(),
                .desired_retention = level["desired_retention"].toDouble()
            };

            if (!level["display_name"].isNull()) {
                retentionClass.display_name = level["display_name"].toString();
            }

            metadata.flashcard_retention_classes.push_back(retentionClass);
        }

        for (const QJsonValue &value : object["systemtask_session_presets"].toArray()) {
            const QJsonObject preset = value.toObject();
            metadata.systemtask_session_presets.push_back({
                .id = preset["id"].toInt(),
                .name = preset["name"].toString(),
                .planned_duration_minutes = preset["planned_duration_minutes"].toInt()
            });
        }

        AppState::metadata = metadata;
        AppState::metadataLoaded = true;
        emit metadataReceived();
        reply->deleteLater();
    });
}

void ApiClient::getDomainTree() {
    const QNetworkRequest networkRequest(buildUrl("/domain-tree"));
    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QJsonObject object = QJsonDocument::fromJson(reply->readAll()).object();
        const std::vector<MetadataItem> domains = parseMetadataItems(object["domains"].toArray());
        std::vector<DomainNode> nodes;

        for (const QJsonValue &value : object["nodes"].toArray()) {
            nodes.push_back(parseDomainNode(value.toObject()));
        }

        emit domainTreeReceived(domains, nodes);
        reply->deleteLater();
    });
}

void ApiClient::createDomainNode(const int parentId, const QString &name) {
    QNetworkRequest networkRequest(buildUrl(
        "/domain-nodes/" + QString::number(parentId) + "/children"
    ));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["name"] = name;
    QNetworkReply *reply = manager.post(networkRequest, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 201) {
            const QJsonObject object = QJsonDocument::fromJson(reply->readAll()).object();
            emit domainNodeCreated(parseDomainNode(object));
        }

        reply->deleteLater();
    });
}

void ApiClient::createRootDomainNode(const int domainId, const QString &name) {
    QNetworkRequest networkRequest(buildUrl(
        "/domains/" + QString::number(domainId) + "/nodes"
    ));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["name"] = name;
    QNetworkReply *reply = manager.post(networkRequest, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 201) {
            const QJsonObject object = QJsonDocument::fromJson(reply->readAll()).object();
            emit domainNodeCreated(parseDomainNode(object));
        }

        reply->deleteLater();
    });
}

void ApiClient::updateDomainNode(
    const int id,
    const QString &name,
    const std::optional<int> cognitiveLevelId,
    const std::optional<int> completenessLevelId,
    const std::optional<int> flashcardStatusId,
    const std::optional<int> verificationStatusId,
    const std::optional<int> verificationLevelId
) {
    QNetworkRequest networkRequest(buildUrl("/domain-nodes/" + QString::number(id)));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["name"] = name;
    json["cognitive_level_id"] = cognitiveLevelId.has_value()
        ? QJsonValue(*cognitiveLevelId)
        : QJsonValue(QJsonValue::Null);
    json["completeness_level_id"] = completenessLevelId.has_value()
        ? QJsonValue(*completenessLevelId) : QJsonValue(QJsonValue::Null);
    json["flashcard_status_id"] = flashcardStatusId.has_value()
        ? QJsonValue(*flashcardStatusId) : QJsonValue(QJsonValue::Null);
    json["verification_status_id"] = verificationStatusId.has_value()
        ? QJsonValue(*verificationStatusId) : QJsonValue(QJsonValue::Null);
    json["verification_level_id"] = verificationLevelId.has_value()
        ? QJsonValue(*verificationLevelId) : QJsonValue(QJsonValue::Null);
    QNetworkReply *reply = manager.sendCustomRequest(
        networkRequest,
        "PATCH",
        QJsonDocument(json).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200) {
            const QJsonObject object = QJsonDocument::fromJson(reply->readAll()).object();
            emit domainNodeUpdated(parseDomainNode(object));
        }

        reply->deleteLater();
    });
}

void ApiClient::createDocument(const int nodeId) {
    QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/document"
    ));
    QNetworkReply *reply = manager.post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError) {
            emit documentCreated(parseDocument(
                QJsonDocument::fromJson(reply->readAll()).object()
            ));
        }

        reply->deleteLater();
    });
}

void ApiClient::getDocument(const int nodeId) {
    const QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/document"
    ));
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError) {
            emit documentReceived(parseDocument(
                QJsonDocument::fromJson(reply->readAll()).object()
            ));
        }

        reply->deleteLater();
    });
}

void ApiClient::updateDocument(const int nodeId, const QString &content) {
    QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/document"
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["content"] = content;
    QNetworkReply *reply = manager.put(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        emit documentUpdated(
            reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200
        );
        reply->deleteLater();
    });
}

void ApiClient::getDueFlashcards() {
    const QNetworkRequest networkRequest(buildUrl(PATH_FLASHCARDS));

    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        std::vector<FlashcardReviewQueueItem> flashcards;

        const QByteArray data = reply->readAll();
        const QJsonArray jsonArray = QJsonDocument::fromJson(data).array();

        for (const QJsonValue &value: jsonArray) {
            FlashcardReviewQueueItem flashcard;
            QJsonObject object = value.toObject();

            flashcard.id = object["id"].toInt();
            flashcard.front = object["front"].toString();
            flashcard.back = object["back"].toString();
            flashcard.review_state_id = static_cast<int16_t>(object["review_state_id"].toInt());
            flashcard.learning_step = static_cast<int16_t>(object["learning_step"].toInt());
            flashcard.retention_class_id = object["retention_class_id"].toInt();

            if (!object["stability"].isNull()) {
                flashcard.stability = static_cast<float>(object["stability"].toDouble());
            }

            if (!object["difficulty"].isNull()) {
                flashcard.difficulty = static_cast<float>(object["difficulty"].toDouble());
            }

            const QJsonObject preview = object["preview"].toObject();
            flashcard.preview.again = static_cast<float>(preview["again"].toDouble());
            flashcard.preview.hard = static_cast<float>(preview["hard"].toDouble());
            flashcard.preview.good = static_cast<float>(preview["good"].toDouble());
            flashcard.preview.easy = static_cast<float>(preview["easy"].toDouble());

            flashcards.push_back(flashcard);
        }

        emit flashcardsReceived(flashcards);

        reply->deleteLater();
    });
}

void ApiClient::rateFlashcard(const int id, const ReviewFlashcard reviewFlashcard) {
    QNetworkRequest networkRequest(buildUrl("/flashcards/" + QString::number(id) + "/reviews"));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["rating"] = reviewFlashcard.rating;
    json["duration_ms"] = reviewFlashcard.duration_ms;

    QNetworkReply *reply = manager.post(networkRequest, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 201) {
            emit flashcardRated();
        } else {
            qWarning() << "Flashcard review failed: " << reply->errorString();
        }

        reply->deleteLater();
    });
}

void ApiClient::getTasks() {
    const QNetworkRequest networkRequest(buildUrl(PATH_TASKS));

    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        std::vector<Task> tasks;

        const QByteArray data = reply->readAll();
        const QJsonArray jsonArray = QJsonDocument::fromJson(data).array();

        for (const QJsonValue &value: jsonArray) {
            Task task;
            QJsonObject object = value.toObject();

            task.id = object["id"].toInt();
            task.name = object["name"].toString();
            task.group_id = object["group_id"].toInt();
            task.state_id = object["state_id"].toInt();
            task.priority_id = object["priority_id"].toInt();

            tasks.push_back(task);
        }

        emit taskReceived(tasks);

        reply->deleteLater();
    });
}

void ApiClient::createTask(const CreateTask &task) {
    QNetworkRequest networkRequest(buildUrl(PATH_TASKS));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["name"] = task.name;
    json["group_id"] = task.group_id;
    json["priority_id"] = task.priority_id;

    QNetworkReply *reply = manager.post(networkRequest, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 201) {
            emit taskCreated();
        } else {
            qWarning() << "Task creation failed:" << reply->errorString();
        }

        reply->deleteLater();
    });
}

void ApiClient::updateTask(const int id, const UpdateTask &task) {
    QNetworkRequest networkRequest(buildUrl("/tasks/" + QString::number(id)));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["name"] = task.name;
    json["state_id"] = task.state_id;
    json["priority_id"] = task.priority_id;

    QNetworkReply *reply = manager.sendCustomRequest(
        networkRequest,
        "PATCH",
        QJsonDocument(json).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 204) {
            emit taskUpdated();
        } else {
            qWarning() << "Task update failed:" << reply->errorString();
        }

        reply->deleteLater();
    });
}

void ApiClient::deleteTask(const int id) {
    const QNetworkRequest networkRequest(buildUrl("/tasks/" + QString::number(id)));
    QNetworkReply *reply = manager.deleteResource(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 204) {
            emit taskDeleted();
        } else {
            qWarning() << "Task deletion failed:" << reply->errorString();
        }

        reply->deleteLater();
    });
}

void ApiClient::getCalendarEntries() {
    const QNetworkRequest request(buildUrl(PATH_CALENDAR_ENTRIES));
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError) {
            std::vector<CalendarEntry> entries;
            for (const QJsonValue &value : QJsonDocument::fromJson(reply->readAll()).array()) {
                const QJsonObject object = value.toObject();
                CalendarEntry entry{
                    .id = object["id"].toInt(),
                    .name = object["name"].toString(),
                    .startAt = QDateTime::fromString(object["start_at"].toString(), Qt::ISODate)
                };

                if (!object["end_at"].isNull()) {
                    entry.endAt = QDateTime::fromString(object["end_at"].toString(), Qt::ISODate);
                }
                if (!object["systemtask_id"].isNull()) {
                    entry.systemTaskId = object["systemtask_id"].toInt();
                }

                entries.push_back(entry);
            }

            emit calendarEntriesReceived(entries);
        } else {
            qWarning() << "Loading calendar entries failed:" << reply->errorString();
        }

        reply->deleteLater();
    });
}

void ApiClient::createCalendarEntry(const CreateCalendarEntry &entry) {
    QNetworkRequest request(buildUrl(PATH_CALENDAR_ENTRIES));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["name"] = entry.name;
    json["start_at"] = entry.startAt.toUTC().toString(Qt::ISODate);
    json["end_at"] = entry.endAt.has_value()
        ? QJsonValue(entry.endAt->toUTC().toString(Qt::ISODate))
        : QJsonValue(QJsonValue::Null);
    json["is_reminder"] = entry.isReminder;
    json["systemtask_id"] = entry.systemTaskId.has_value()
        ? QJsonValue(*entry.systemTaskId)
        : QJsonValue(QJsonValue::Null);

    QNetworkReply *reply = manager.post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 201) {
            emit calendarEntryCreated();
        } else {
            qWarning() << "Calendar entry creation failed:" << reply->errorString();
        }

        reply->deleteLater();
    });
}
