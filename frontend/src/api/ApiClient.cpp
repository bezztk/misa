#include "ApiClient.h"
#include "../models/AppState.h"
#include "../models/Task.h"
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
        .flashcardCount = object["flashcard_count"].toInt()
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
            flashcard.state = static_cast<int16_t>(object["state"].toInt());
            flashcard.learning_step = static_cast<int16_t>(object["learning_step"].toInt());
            flashcard.retention_levels_id = object["retention_levels_id"].toInt();

            if (!object["stability"].isNull()) {
                flashcard.stability = static_cast<float>(object["stability"].toDouble());
            }

            if (!object["difficulty"].isNull()) {
                flashcard.difficulty = static_cast<float>(object["difficulty"].toDouble());
            }

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

void ApiClient::getFlashcardPreview(const int id) {
    const QNetworkRequest networkRequest(buildUrl("/flashcards/" + QString::number(id) + "/preview"));

    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        FlashcardReviewPreview preview{};
        const QByteArray data = reply->readAll();
        const QJsonObject jsonObject = QJsonDocument::fromJson(data).object();

        preview.again = static_cast<float>(jsonObject["again"].toDouble());
        preview.hard = static_cast<float>(jsonObject["hard"].toDouble());
        preview.good = static_cast<float>(jsonObject["good"].toDouble());
        preview.easy = static_cast<float>(jsonObject["easy"].toDouble());

        emit flashcardPreviewCalculated(preview);

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
