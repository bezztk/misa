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
                if (requestSucceeded(reply)
                    || reply->request().attribute(HANDLE_ERROR_LOCALLY).toBool()) {
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

    if (!reply->request().attribute(HANDLE_ERROR_LOCALLY).toBool()) {
        emit requestFailed(operation + ": Ungültige Serverantwort");
    }
    return false;
}

void ApiClient::createComment(const int entityId, const QString &content) {
    QNetworkRequest request(buildUrl(
        "/entities/" + QString::number(entityId) + "/comments"
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.post(
        request,
        QJsonDocument(QJsonObject{{"content", content}}).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply, entityId] {
        QJsonDocument response;
        if (readJson(reply, "Kommentar erstellen", response) && response.isObject()) {
            emit commentCreated(entityId, parseComment(response.object()));
        }
        reply->deleteLater();
    });
}

void ApiClient::updateComment(const int id, const QString &content) {
    QNetworkRequest request(buildUrl("/comments/" + QString::number(id)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(QJsonObject{{"content", content}}).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (readJson(reply, "Kommentar aktualisieren", response) && response.isObject()) {
            emit commentUpdated(parseComment(response.object()));
        }
        reply->deleteLater();
    });
}

void ApiClient::deleteComment(const int id) {
    const QNetworkRequest request(buildUrl("/comments/" + QString::number(id)));
    QNetworkReply *reply = manager.deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, id] {
        if (requestSucceeded(reply)) {
            emit commentDeleted(id);
        }
        reply->deleteLater();
    });
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
    if (!object["description"].isNull()) {
        item.description = object["description"].toString();
    }
    if (!object["color"].isNull()) {
        item.color = object["color"].toString();
    }
    if (!object["icon"].isNull()) {
        QByteArray icon;
        for (const QJsonValue &byte : object["icon"].toArray()) {
            icon.append(static_cast<char>(byte.toInt()));
        }
        item.icon = icon;
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
        .attachmentCount = object["attachment_count"].toInt(),
        .skillCount = object["skill_count"].toInt(),
        .pursuitCount = object["pursuit_count"].toInt(),
        .taskCount = object["task_count"].toInt()
    };
    node.comments = parseComments(object["comments"].toArray());

    if (!object["parent_id"].isNull()) {
        node.parentId = object["parent_id"].toInt();
    }
    if (!object["knowledge_level_id"].isNull()) {
        node.knowledgeLevelId = object["knowledge_level_id"].toInt();
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
                .back = object["back"].toString(),
                .comments = parseComments(object["comments"].toArray())
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
            emit flashcardCreated();
        }
        reply->deleteLater();
    });
}

void ApiClient::updateFlashcardField(
    const int nodeId,
    const int id,
    const FlashcardField field,
    const QJsonValue &value
) {
    QNetworkRequest request(buildUrl(
        "/flashcards/" + QString::number(id)
        + "/fields/" + QString::number(static_cast<int>(field))
    ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject json{{"value", value}};
    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(json).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (requestSucceeded(reply)) {
            emit nodeFlashcardsChanged(nodeId);
            emit flashcardUpdated();
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
        for (const QJsonValue &value : object["knowledge_levels"].toArray()) {
            const QJsonObject level = value.toObject();
            metadata.knowledgeLevels.push_back({
                .id = level["id"].toInt(),
                .name = level["name"].toString(),
                .displayName = level["display_name"].toString(),
                .description = level["description"].toString()
            });
        }
        metadata.psychomotorLevels = parseMetadataItems(object["psychomotor_levels"].toArray());
        metadata.documentCompletenessLevels = parseMetadataItems(object["document_completeness_levels"].toArray());
        metadata.documentFlashcardStatuses = parseMetadataItems(object["document_flashcard_statuses"].toArray());
        metadata.documentVerificationStatuses = parseMetadataItems(object["document_verification_statuses"].toArray());
        metadata.documentVerificationLevels = parseMetadataItems(object["document_verification_levels"].toArray());
        metadata.ideaPhases = parseMetadataItems(object["idea_phases"].toArray());
        metadata.ideaStates = parseMetadataItems(object["idea_states"].toArray());
        metadata.mediaRatings = parseMetadataItems(object["media_ratings"].toArray());
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

void ApiClient::getEntityFieldVersions(const int entityId) {
    QNetworkReply *reply = manager.get(QNetworkRequest(buildUrl(
        "/entities/" + QString::number(entityId) + "/versions"
    )));
    connect(reply, &QNetworkReply::finished, this, [this, reply, entityId] {
        QJsonDocument response;
        if (!readJson(reply, "Versionshistorie laden", response) || !response.isArray()) {
            if (!response.isNull() && !response.isArray()) {
                emit requestFailed("Versionshistorie laden: Unerwartetes Datenformat");
            }
            emit entityFieldVersionsLoadFailed(entityId);
            reply->deleteLater();
            return;
        }

        std::vector<EntityFieldVersion> versions;
        for (const QJsonValue &value : response.array()) {
            const QJsonObject object = value.toObject();
            versions.push_back({
                .version = object["version"].toInt(),
                .fieldName = object["field_name"].toString(),
                .oldValue = object["old_value"],
                .newValue = object["new_value"],
                .changedAt = QDateTime::fromString(
                    object["changed_at"].toString(),
                    Qt::ISODate
                )
            });
        }
        emit entityFieldVersionsReceived(entityId, versions);
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

void ApiClient::updateDomain(
    const int id,
    const std::optional<QString> &color,
    const std::optional<QByteArray> &icon
) {
    QNetworkRequest request(buildUrl("/domains/" + QString::number(id)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["color"] = color ? QJsonValue(*color) : QJsonValue::Null;
    if (icon) {
        QJsonArray bytes;
        for (const char byte : *icon) {
            bytes.append(static_cast<unsigned char>(byte));
        }
        json["icon"] = bytes;
    } else {
        json["icon"] = QJsonValue::Null;
    }

    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(json).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (readJson(reply, "Domäne aktualisieren", response) && response.isObject()) {
            emit domainUpdated(parseMetadataItem(response.object()));
        }
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

void ApiClient::updateDomainNodeField(
    const int id,
    const DomainNodeField field,
    const QJsonValue &value
) {
    QNetworkRequest networkRequest(buildUrl(
        "/domain-nodes/" + QString::number(id)
        + "/fields/" + QString::number(static_cast<int>(field))
    ));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["value"] = value;
    QNetworkReply *reply = manager.sendCustomRequest(
        networkRequest,
        "PATCH",
        QJsonDocument(json).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply, id, field, value] {
        if (requestSucceeded(reply)) {
            emit domainNodeFieldUpdated(id, field, value);
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
        if (!readJson(reply, "Aufschrieb erstellen", response) || !response.isObject()) {
            if (!response.isNull() && !response.isObject()) {
                emit requestFailed("Aufschrieb erstellen: Unerwartetes Datenformat");
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
        if (!readJson(reply, "Aufschrieb laden", response) || !response.isObject()) {
            if (!response.isNull() && !response.isObject()) {
                emit requestFailed("Aufschrieb laden: Unerwartetes Datenformat");
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

void ApiClient::deleteDocument(const int nodeId) {
    const QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/document"
    ));
    QNetworkReply *reply = manager.deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        if (requestSucceeded(reply)) {
            emit documentDeleted(nodeId);
        }
        reply->deleteLater();
    });
}

void ApiClient::getDocumentVersions(const int nodeId) {
    QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId) + "/document/versions"
    ));
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        QJsonDocument response;
        if (!readJson(reply, "Aufschrieb-Versionshistorie laden", response)
            || !response.isArray()) {
            emit documentVersionsLoadFailed(nodeId);
            reply->deleteLater();
            return;
        }

        std::vector<DocumentVersion> versions;
        for (const QJsonValue &value : response.array()) {
            const QJsonObject object = value.toObject();
            versions.push_back({
                .version = object["version"].toInt(),
                .content = object["content"].toString(),
                .capturedAt = QDateTime::fromString(
                    object["captured_at"].toString(),
                    Qt::ISODate
                )
            });
        }
        emit documentVersionsReceived(nodeId, versions);
        reply->deleteLater();
    });
}

void ApiClient::restoreDocumentVersion(const int nodeId, const int version) {
    QNetworkRequest request(buildUrl(
        "/domain-nodes/" + QString::number(nodeId)
        + "/document/versions/" + QString::number(version) + "/restore"
    ));
    QNetworkReply *reply = manager.post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply, nodeId] {
        QJsonDocument response;
        if (!readJson(reply, "Aufschrieb-Version wiederherstellen", response)
            || !response.isObject()) {
            emit documentVersionRestoreFailed(nodeId);
            reply->deleteLater();
            return;
        }

        emit documentVersionRestored(parseDocument(response.object()));
        reply->deleteLater();
    });
}

void ApiClient::getDueFlashcards() {
    requestDueFlashcards(true);
}

void ApiClient::getDueFlashcardCount() {
    requestDueFlashcards(false);
}

void ApiClient::requestDueFlashcards(const bool includeFlashcards) {
    const QNetworkRequest networkRequest(buildUrl(PATH_FLASHCARDS));

    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, includeFlashcards] {
        QJsonDocument response;
        if (!readJson(reply, "Lernkarten laden", response) || !response.isArray()) {
            if (!response.isNull() && !response.isArray()) {
                emit requestFailed("Lernkarten laden: Unerwartetes Datenformat");
            }
            emit dueFlashcardsLoadFailed();
            reply->deleteLater();
            return;
        }

        const QJsonArray jsonArray = response.array();
        emit dueFlashcardCountReceived(jsonArray.size());

        if (!includeFlashcards) {
            reply->deleteLater();
            return;
        }

        std::vector<FlashcardReviewQueueItem> flashcards;

        for (const QJsonValue &value: jsonArray) {
            FlashcardReviewQueueItem flashcard{};
            QJsonObject object = value.toObject();

            flashcard.id = object["id"].toInt();
            flashcard.front = object["front"].toString();
            flashcard.back = object["back"].toString();
            flashcard.reviewStateId = static_cast<std::int16_t>(object["review_state_id"].toInt());
            flashcard.learningStep = static_cast<std::int16_t>(object["learning_step"].toInt());
            flashcard.retentionClassId = object["retention_class_id"].toInt();
            flashcard.comments = parseComments(object["comments"].toArray());

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
