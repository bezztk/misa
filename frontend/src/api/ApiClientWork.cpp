#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

void ApiClient::getTasks() {
    const QNetworkRequest networkRequest(buildUrl(PATH_TASKS));

    QNetworkReply *reply = manager.get(networkRequest);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QJsonDocument response;
        if (!readJson(reply, "Aufgaben laden", response) || !response.isArray()) {
            if (!response.isNull() && !response.isArray()) {
                emit requestFailed("Aufgaben laden: Unerwartetes Datenformat");
            }
            emit tasksLoadFailed();
            reply->deleteLater();
            return;
        }

        std::vector<Task> tasks;

        const QJsonArray jsonArray = response.array();

        for (const QJsonValue &value: jsonArray) {
            Task task{};
            QJsonObject object = value.toObject();

            task.id = object["id"].toInt();
            task.name = object["name"].toString();
            if (!object["group_id"].isNull()) {
                task.groupId = object["group_id"].toInt();
            }
            if (!object["domain_node_id"].isNull()) {
                task.domainNodeId = object["domain_node_id"].toInt();
            }
            task.stateId = object["state_id"].toInt();
            task.priorityId = object["priority_id"].toInt();
            task.comments = parseComments(object["comments"].toArray());

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
    json["group_id"] = task.groupId.has_value()
        ? QJsonValue(*task.groupId)
        : QJsonValue(QJsonValue::Null);
    json["priority_id"] = task.priorityId;
    json["domain_node_id"] = task.domainNodeId.has_value()
        ? QJsonValue(*task.domainNodeId)
        : QJsonValue(QJsonValue::Null);

    QNetworkReply *reply = manager.post(networkRequest, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 201) {
            emit taskCreated();
        }

        reply->deleteLater();
    });
}

void ApiClient::updateTaskField(
    const int id,
    const TaskField field,
    const QJsonValue &value
) {
    QNetworkRequest networkRequest(buildUrl(
        "/tasks/" + QString::number(id)
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

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 204) {
            emit taskUpdated();
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
        }

        reply->deleteLater();
    });
}

void ApiClient::getCalendarEntries() {
    const QNetworkRequest request(buildUrl(PATH_CALENDAR_ENTRIES));
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QJsonDocument response;
        if (!readJson(reply, "Kalendereinträge laden", response) || !response.isArray()) {
            if (!response.isNull() && !response.isArray()) {
                emit requestFailed("Kalendereinträge laden: Unerwartetes Datenformat");
            }
            reply->deleteLater();
            return;
        }

        std::vector<CalendarEntry> entries;
        for (const QJsonValue &value : response.array()) {
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
            if (!object["routine_execution_id"].isNull()) {
                entry.routineExecutionId = object["routine_execution_id"].toInt();
            }
            entry.isLocked = object["is_locked"].toBool();
            entries.push_back(entry);
        }

        emit calendarEntriesReceived(entries);
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
    json["is_locked"] = entry.isLocked;

    QNetworkReply *reply = manager.post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 201) {
            emit calendarEntryCreated();
        }

        reply->deleteLater();
    });
}

void ApiClient::updateCalendarEntry(const int id, const CreateCalendarEntry &entry) {
    QNetworkRequest request(buildUrl("/calendar-entries/" + QString::number(id)));
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
    json["is_locked"] = entry.isLocked;

    QNetworkReply *reply = manager.sendCustomRequest(
        request,
        "PATCH",
        QJsonDocument(json).toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this, reply, id] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 204) {
            emit calendarEntryUpdated();
        } else {
            emit calendarEntryUpdateFailed(id);
        }

        reply->deleteLater();
    });
}

void ApiClient::deleteCalendarEntry(const int id) {
    const QNetworkRequest request(
        buildUrl("/calendar-entries/" + QString::number(id))
    );
    QNetworkReply *reply = manager.deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 204) {
            emit calendarEntryDeleted();
        }
        reply->deleteLater();
    });
}

