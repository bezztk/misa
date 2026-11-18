#include "ApiClient.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ApiClient::ApiClient(QObject *parent) : QObject(parent) {
    connect(&manager, &QNetworkAccessManager::finished, this,
            [this](QNetworkReply *reply) {
                if (requestSucceeded(reply)) {
                    return;
                }

                const int status = reply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute
                ).toInt();
                const QString detail = status > 0
                    ? QString("HTTP %1").arg(status)
                    : reply->errorString();
                emit requestFailed(
                    QString("%1: %2").arg(reply->request().url().path(), detail)
                );
            });
}

const Metadata &ApiClient::metadata() const {
    return loadedMetadata;
}

bool ApiClient::requestSucceeded(const QNetworkReply *reply) const {
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return reply->error() == QNetworkReply::NoError && status >= 200 && status < 300;
}

bool ApiClient::readJson(
    QNetworkReply *reply,
    const QString &operation,
    QJsonDocument &document
) {
    if (!requestSucceeded(reply)) {
        return false;
    }

    QJsonParseError error;
    document = QJsonDocument::fromJson(reply->readAll(), &error);
    if (error.error == QJsonParseError::NoError && !document.isNull()) {
        return true;
    }

    emit requestFailed(operation + ": Ungültige Serverantwort");
    return false;
}

static MetadataItem parseMetadataItem(const QJsonValue &value) {
    const QJsonObject object = value.toObject();
    MetadataItem item{
        .id = object["id"].toInt(),
        .name = object["name"].toString()
    };

    if (!object["display_name"].isNull()) {
        item.displayName = object["display_name"].toString();
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
        .hasRoutine = object["has_routine"].toBool(),
        .flashcardCount = object["flashcard_count"].toInt(),
        .attachmentCount = object["attachment_count"].toInt(),
        .skillCount = object["skill_count"].toInt(),
        .pursuitCount = object["pursuit_count"].toInt(),
        .taskCount = object["task_count"].toInt()
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

static Routine parseRoutine(const QJsonObject &object) {
    Routine routine{
        .id = object["id"].toInt(),
        .systemTaskId = object["systemtask_id"].toInt(),
        .domainNodeId = object["domain_node_id"].toInt(),
        .name = object["name"].toString(),
        .recurrenceRuleSet = object["recurrence_rule_set"].toString(),
        .timezone = object["timezone"].toString(),
        .isOptional = object["is_optional"].toBool(),
        .isActive = object["is_active"].toBool()
    };
    if (!object["estimated_duration"].isNull()) {
        routine.estimatedDuration = object["estimated_duration"].toInt();
    }
    return routine;
}

static QJsonObject routineJson(const SaveRoutine &routine) {
    QJsonObject json{
        {"name", routine.name},
        {"recurrence_rule_set", routine.recurrenceRuleSet},
        {"timezone", routine.timezone},
        {"is_optional", routine.isOptional},
        {"is_active", routine.isActive}
    };
    json["estimated_duration"] = routine.estimatedDuration.has_value()
        ? QJsonValue(*routine.estimatedDuration)
        : QJsonValue(QJsonValue::Null);
    return json;
}

void ApiClient::getNodeFlashcards(const int nodeId) {
    const QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/flashcards"
    ));
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        QJsonDocument response;
        if (!readJson(reply, "Lernkarten laden", response) || !response.isArray()) {
            if (!response.isNull() && !response.isArray()) {
                emit requestFailed("Lernkarten laden: Unerwartetes Datenformat");
            }
            reply->deleteLater();
            return;
        }

        std::vector<Flashcard> flashcards;
        for (const QJsonValue &value : response.array()) {
            const QJsonObject object = value.toObject();
            flashcards.push_back({
                .id = object["id"].toInt(),
                .front = object["front"].toString(),
                .back = object["back"].toString()
            });
        }
        emit nodeFlashcardsReceived(nodeId, flashcards);
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
        if (requestSucceeded(reply)) {
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
        if (requestSucceeded(reply)) {
            emit nodeFlashcardsChanged(nodeId);
        }
        reply->deleteLater();
    });
}

void ApiClient::deleteFlashcard(const int nodeId, const int id) {
    const QNetworkRequest request(buildUrl("/flashcards/" + QString::number(id)));
    QNetworkReply *reply = manager.deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (requestSucceeded(reply)) {
            emit nodeFlashcardsChanged(nodeId);
        }
        reply->deleteLater();
    });
}

void ApiClient::getNodeAttachments(const int nodeId) {
    const QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/attachments"
    ));
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        QJsonDocument response;
        if (!readJson(reply, "Anhänge laden", response) || !response.isArray()) {
            reply->deleteLater();
            return;
        }

        std::vector<Attachment> attachments;
        for (const QJsonValue &value : response.array()) {
            const QJsonObject object = value.toObject();
            attachments.push_back({
                .id = object["id"].toInt(),
                .domainNodeId = object["domain_node_id"].toInt(),
                .originalName = object["original_name"].toString(),
                .storageName = object["storage_name"].toString(),
                .sizeBytes = static_cast<qint64>(object["size_bytes"].toDouble()),
                .createdAt = object["created_at"].toString()
            });
        }
        emit nodeAttachmentsReceived(nodeId, attachments);
        reply->deleteLater();
    });
}

void ApiClient::createAttachment(
    const int nodeId,
    const QString &originalName,
    const QString &storageName,
    const qint64 sizeBytes
) {
    QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/attachments"
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject json{
        {"original_name", originalName},
        {"storage_name", storageName},
        {"size_bytes", sizeBytes}
    };
    QNetworkReply *reply = manager.post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (requestSucceeded(reply)) {
            emit nodeAttachmentsChanged(nodeId);
        }
        reply->deleteLater();
    });
}

void ApiClient::deleteAttachment(const int nodeId, const int id) {
    const QNetworkRequest request(buildUrl("/attachments/" + QString::number(id)));
    QNetworkReply *reply = manager.deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (requestSucceeded(reply)) {
            emit nodeAttachmentsChanged(nodeId);
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
    if (metadataLoaded) {
        emit metadataReceived();
        return;
    }
    if (metadataLoading) {
        return;
    }

    metadataLoading = true;
    const QNetworkRequest networkRequest(buildUrl(PATH_METADATA));
    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        metadataLoading = false;
        QJsonDocument response;
        if (!readJson(reply, "Metadaten laden", response) || !response.isObject()) {
            if (!response.isNull() && !response.isObject()) {
                emit requestFailed("Metadaten laden: Unerwartetes Datenformat");
            }
            reply->deleteLater();
            return;
        }

        const QJsonObject object = response.object();
        Metadata metadata{};

        metadata.personalRelevanceLevels = parseMetadataItems(object["personal_relevance_levels"].toArray());
        metadata.lastingImpacts = parseMetadataItems(object["lasting_impacts"].toArray());
        metadata.cognitiveLevels = parseMetadataItems(object["cognitive_levels"].toArray());
        metadata.psychomotorLevels = parseMetadataItems(object["psychomotor_levels"].toArray());
        metadata.documentCompletenessLevels = parseMetadataItems(object["document_completeness_levels"].toArray());
        metadata.documentFlashcardStatuses = parseMetadataItems(object["document_flashcard_statuses"].toArray());
        metadata.documentVerificationStatuses = parseMetadataItems(object["document_verification_statuses"].toArray());
        metadata.documentVerificationLevels = parseMetadataItems(object["document_verification_levels"].toArray());
        metadata.systemTaskGroups = parseMetadataItems(object["systemtask_groups"].toArray());
        metadata.systemTaskStates = parseMetadataItems(object["systemtask_states"].toArray());
        metadata.systemTaskPriorities = parseMetadataItems(object["systemtask_priorities"].toArray());

        for (const QJsonValue &value : object["flashcard_retention_classes"].toArray()) {
            const QJsonObject level = value.toObject();
            FlashcardRetentionClass retentionClass{
                .id = level["id"].toInt(),
                .name = level["name"].toString(),
                .desiredRetention = level["desired_retention"].toDouble()
            };

            if (!level["display_name"].isNull()) {
                retentionClass.displayName = level["display_name"].toString();
            }

            metadata.flashcardRetentionClasses.push_back(retentionClass);
        }

        loadedMetadata = std::move(metadata);
        metadataLoaded = true;
        emit metadataReceived();
        reply->deleteLater();
    });
}

void ApiClient::getDomainTree() {
    const QNetworkRequest networkRequest(buildUrl("/domain-tree"));
    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (!readJson(reply, "Domänen laden", response) || !response.isObject()) {
            if (!response.isNull() && !response.isObject()) {
                emit requestFailed("Domänen laden: Unerwartetes Datenformat");
            }
            reply->deleteLater();
            return;
        }
        const QJsonObject object = response.object();
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
        QJsonDocument response;
        if (readJson(reply, "Bereich konnte nicht erstellt werden", response)
            && response.isObject()) {
            emit domainNodeCreated(parseDomainNode(response.object()));
        } else if (!response.isNull() && !response.isObject()) {
            emit requestFailed("Bereich konnte nicht erstellt werden: Ungültige Serverantwort");
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
        QJsonDocument response;
        if (readJson(reply, "Wurzelbereich konnte nicht erstellt werden", response)
            && response.isObject()) {
            emit domainNodeCreated(parseDomainNode(response.object()));
        } else if (!response.isNull() && !response.isObject()) {
            emit requestFailed("Wurzelbereich konnte nicht erstellt werden: Ungültige Serverantwort");
        }

        reply->deleteLater();
    });
}

void ApiClient::deleteDomainNode(const int id) {
    const QNetworkRequest request(buildUrl("/domain-nodes/" + QString::number(id)));
    QNetworkReply *reply = manager.deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, id] {
        if (requestSucceeded(reply)) {
            emit domainNodeDeleted(id);
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
        QJsonDocument response;
        if (readJson(reply, "Bereich konnte nicht aktualisiert werden", response)
            && response.isObject()) {
            emit domainNodeUpdated(parseDomainNode(response.object()));
        } else if (!response.isNull() && !response.isObject()) {
            emit requestFailed("Bereich konnte nicht aktualisiert werden: Ungültige Serverantwort");
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
        QJsonDocument response;
        if (!readJson(reply, "Dokument erstellen", response) || !response.isObject()) {
            if (!response.isNull() && !response.isObject()) {
                emit requestFailed("Dokument erstellen: Unerwartetes Datenformat");
            }
            reply->deleteLater();
            return;
        }

        emit documentCreated(parseDocument(response.object()));
        reply->deleteLater();
    });
}

void ApiClient::getDocument(const int nodeId) {
    const QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/document"
    ));
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        QJsonDocument response;
        if (!readJson(reply, "Dokument laden", response) || !response.isObject()) {
            if (!response.isNull() && !response.isObject()) {
                emit requestFailed("Dokument laden: Unerwartetes Datenformat");
            }
            emit documentLoadFailed(nodeId);
            reply->deleteLater();
            return;
        }

        emit documentReceived(parseDocument(response.object()));
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

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        emit documentUpdated(
            nodeId,
            requestSucceeded(reply)
        );
        reply->deleteLater();
    });
}

void ApiClient::getDueFlashcards() {
    const QNetworkRequest networkRequest(buildUrl(PATH_FLASHCARDS));

    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (!readJson(reply, "Lernkarten laden", response) || !response.isArray()) {
            if (!response.isNull() && !response.isArray()) {
                emit requestFailed("Lernkarten laden: Unerwartetes Datenformat");
            }
            reply->deleteLater();
            return;
        }

        std::vector<FlashcardReviewQueueItem> flashcards;

        const QJsonArray jsonArray = response.array();

        for (const QJsonValue &value: jsonArray) {
            FlashcardReviewQueueItem flashcard{};
            QJsonObject object = value.toObject();

            flashcard.id = object["id"].toInt();
            flashcard.front = object["front"].toString();
            flashcard.back = object["back"].toString();
            flashcard.reviewStateId = static_cast<std::int16_t>(object["review_state_id"].toInt());
            flashcard.learningStep = static_cast<std::int16_t>(object["learning_step"].toInt());
            flashcard.retentionClassId = object["retention_class_id"].toInt();

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
    json["duration_ms"] = reviewFlashcard.durationMs;

    QNetworkReply *reply = manager.post(networkRequest, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit flashcardRated();
        } else {
            emit flashcardRatingFailed();
        }

        reply->deleteLater();
    });
}

void ApiClient::getDomainNodeRoutine(const int nodeId) {
    const QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/routine"
    ));
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        QJsonDocument response;
        if (readJson(reply, "Routine konnte nicht geladen werden", response)
            && response.isObject()) {
            emit domainNodeRoutineReceived(nodeId, parseRoutine(response.object()));
        } else if (!response.isNull() && !response.isObject()) {
            emit requestFailed("Routine konnte nicht geladen werden: Ungültige Serverantwort");
        }
        reply->deleteLater();
    });
}

void ApiClient::createDomainNodeRoutine(const int nodeId, const SaveRoutine &routine) {
    QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/routine"
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.post(request, QJsonDocument(routineJson(routine)).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        QJsonDocument response;
        if (readJson(reply, "Routine konnte nicht erstellt werden", response)
            && response.isObject()) {
            emit domainNodeRoutineSaved(nodeId, parseRoutine(response.object()));
        } else if (!response.isNull() && !response.isObject()) {
            emit requestFailed("Routine konnte nicht erstellt werden: Ungültige Serverantwort");
        }
        reply->deleteLater();
    });
}

void ApiClient::updateDomainNodeRoutine(const int nodeId, const SaveRoutine &routine) {
    QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/routine"
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.put(request, QJsonDocument(routineJson(routine)).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        QJsonDocument response;
        if (readJson(reply, "Routine konnte nicht aktualisiert werden", response)
            && response.isObject()) {
            emit domainNodeRoutineSaved(nodeId, parseRoutine(response.object()));
        } else if (!response.isNull() && !response.isObject()) {
            emit requestFailed("Routine konnte nicht aktualisiert werden: Ungültige Serverantwort");
        }
        reply->deleteLater();
    });
}

void ApiClient::generateRoutineExecutions() {
    QNetworkRequest request(buildUrl("/routines/generate-executions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (requestSucceeded(reply)) {
            emit routineExecutionsGenerated();
        }
        reply->deleteLater();
    });
}
