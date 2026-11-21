#include "CalendarView.h"
#include "CalendarEntryDialog.h"

#include <QDateTime>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QResizeEvent>
#include <QUrl>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineView>

void CalendarBridge::calendarReady() {
    emit ready();
}

void CalendarBridge::rangeChanged(const QString &start, const QString &end) {
    emit visibleRangeChanged(start, end);
}

void CalendarBridge::timeRangeSelected(const QString &start, const QString &end) {
    emit createEntryForRange(start, end);
}

void CalendarBridge::editEntryRequested(const int id) {
    emit editEntry(id);
}

void CalendarBridge::contextMenuRequested(const int id, const int x, const int y) {
    emit showContextMenu(id, x, y);
}

void CalendarBridge::entryChangeRequested(
    const int id,
    const QString &start,
    const QString &end
) {
    emit changeEntry(id, start, end);
}

CalendarView::CalendarView(ApiClient &client, QWidget *parent)
    : QWidget(parent),
      apiClient(client),
      webView(new QWebEngineView(this)),
      webChannel(new QWebChannel(this)),
      bridge(new CalendarBridge(this)) {
    webView->setGeometry(rect());
    webChannel->registerObject(QStringLiteral("calendarBridge"), bridge);
    webView->page()->setWebChannel(webChannel);

    connect(bridge, &CalendarBridge::ready, this, [this] {
        calendarReady = true;
        pushEntries();
    });
    connect(bridge, &CalendarBridge::visibleRangeChanged, this,
            [this](const QString &, const QString &) {
                apiClient.getCalendarEntries();
            });
    connect(bridge, &CalendarBridge::createEntryForRange, this,
            [this](const QString &start, const QString &end) {
                openCreateDialog(
                    QDateTime::fromString(start, Qt::ISODate),
                    QDateTime::fromString(end, Qt::ISODate)
                );
            });
    connect(bridge, &CalendarBridge::editEntry,
            this, &CalendarView::openEditDialog);
    connect(bridge, &CalendarBridge::showContextMenu,
            this, &CalendarView::showEntryContextMenu);
    connect(bridge, &CalendarBridge::changeEntry,
            this, &CalendarView::updateEntryTime);

    connect(&apiClient, &ApiClient::calendarEntriesReceived, this,
            [this](const std::vector<CalendarEntry> &receivedEntries) {
                entries = receivedEntries;
                pushEntries();
            });
    connect(&apiClient, &ApiClient::calendarEntryCreated, this, [this] {
        apiClient.getCalendarEntries();
    });
    connect(&apiClient, &ApiClient::calendarEntryUpdated, this, [this] {
        apiClient.getCalendarEntries();
    });
    connect(&apiClient, &ApiClient::calendarEntryUpdateFailed,
            bridge, &CalendarBridge::entryChangeFailed);
    connect(&apiClient, &ApiClient::calendarEntryDeleted, this, [this] {
        apiClient.getCalendarEntries();
    });
    connect(&apiClient, &ApiClient::taskDeleted, this, [this] {
        apiClient.getCalendarEntries();
    });
    connect(&apiClient, &ApiClient::routineExecutionsGenerated, this, [this] {
        apiClient.getCalendarEntries();
    });

    webView->setUrl(QUrl(QStringLiteral("qrc:/calendar/index.html")));
}

void CalendarView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    webView->setGeometry(rect());
    emit bridge->resizeRequested();
}

void CalendarView::pushEntries() {
    if (!calendarReady) {
        return;
    }

    QJsonArray calendarEvents;
    for (const CalendarEntry &entry : entries) {
        QJsonObject event;
        event["id"] = QString::number(entry.id);
        event["title"] = entry.name;
        event["start"] = entry.startAt.toLocalTime().toString(Qt::ISODateWithMs);
        event["allDay"] = false;
        event["editable"] = !entry.isLocked;
        event["extendedProps"] = QJsonObject{{"locked", entry.isLocked}};

        if (entry.endAt.has_value()) {
            event["end"] = entry.endAt->toLocalTime().toString(Qt::ISODateWithMs);
        }

        QJsonArray classNames;
        if (entry.isReminder()) {
            classNames.append("misa-reminder");
        }
        if (entry.systemTaskId.has_value()) {
            classNames.append("misa-task");
        } else if (entry.routineExecutionId.has_value()) {
            classNames.append("misa-routine");
        }
        event["classNames"] = classNames;
        calendarEvents.append(event);
    }

    emit bridge->entriesChanged(
        QString::fromUtf8(QJsonDocument(calendarEvents).toJson(QJsonDocument::Compact))
    );
}

void CalendarView::openCreateDialog() {
    const QDateTime initialStart = QDateTime::currentDateTime();
    const auto entry = showCalendarEntryDialog(this, "Kalendereintrag erstellen", {
        .startAt = initialStart,
        .endAt = initialStart.addSecs(3600),
        .isReminder = true
    }, "Erstellen", false);
    if (entry) {
        apiClient.createCalendarEntry(*entry);
    }
}

void CalendarView::refresh() {
    apiClient.generateRoutineExecutions();
}

void CalendarView::openCreateDialog(const QDateTime &start, const QDateTime &end) {
    const auto entry = showCalendarEntryDialog(this, "Kalendereintrag erstellen", {
        .startAt = start,
        .endAt = end,
        .isReminder = false
    }, "Erstellen", false);
    if (entry) {
        apiClient.createCalendarEntry(*entry);
    }
}

void CalendarView::openEditDialog(const int id) {
    const CalendarEntry *entryPointer = findEntry(id);
    if (entryPointer == nullptr || entryPointer->isLocked) {
        return;
    }
    const CalendarEntry entry = *entryPointer;

    const auto updatedEntry = showCalendarEntryDialog(
        this,
        "Kalendereintrag bearbeiten",
        editableEntry(entry),
        "Fertig",
        true
    );
    if (updatedEntry) {
        apiClient.updateCalendarEntry(id, *updatedEntry);
    }
}

void CalendarView::showEntryContextMenu(const int id, const int x, const int y) {
    const CalendarEntry *entryPointer = findEntry(id);
    if (entryPointer == nullptr) {
        return;
    }
    const CalendarEntry entry = *entryPointer;

    QMenu menu(webView);
    QAction *editAction = menu.addAction(
        QIcon(":/assets/icons/edit.png"),
        "Bearbeiten"
    );
    QAction *deleteAction = menu.addAction(
        QIcon(":/assets/icons/delete.png"),
        "Löschen"
    );
    editAction->setEnabled(!entry.isLocked);
    deleteAction->setEnabled(!entry.isLocked);
    menu.addSeparator();
    QAction *lockAction = menu.addAction(
        entry.isLocked ? "Entsperren" : "Sperren"
    );

    QAction *selectedAction = menu.exec(webView->mapToGlobal(QPoint(x, y)));
    if (selectedAction == editAction) {
        openEditDialog(id);
    } else if (selectedAction == deleteAction) {
        apiClient.deleteCalendarEntry(id);
    } else if (selectedAction == lockAction) {
        CreateCalendarEntry updatedEntry = editableEntry(entry);
        updatedEntry.isLocked = !entry.isLocked;
        apiClient.updateCalendarEntry(id, updatedEntry);
    }
}

void CalendarView::updateEntryTime(
    const int id,
    const QString &start,
    const QString &end
) {
    const CalendarEntry *entry = findEntry(id);
    const QDateTime startAt = QDateTime::fromString(start, Qt::ISODate);
    const std::optional<QDateTime> endAt = end.isEmpty()
        ? std::nullopt
        : std::optional(QDateTime::fromString(end, Qt::ISODate));

    if (entry == nullptr || entry->isLocked || !startAt.isValid()
        || (endAt.has_value() && (!endAt->isValid() || *endAt <= startAt))) {
        emit bridge->entryChangeFailed(id);
        return;
    }

    CreateCalendarEntry updatedEntry = editableEntry(*entry);
    updatedEntry.startAt = startAt;
    updatedEntry.endAt = endAt;
    updatedEntry.isReminder = !endAt.has_value();
    apiClient.updateCalendarEntry(id, updatedEntry);
}

const CalendarEntry *CalendarView::findEntry(const int id) const {
    for (const CalendarEntry &entry : entries) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

CreateCalendarEntry CalendarView::editableEntry(const CalendarEntry &entry) {
    return {
        .name = entry.name,
        .startAt = entry.startAt.toLocalTime(),
        .endAt = entry.endAt.has_value()
            ? std::optional(entry.endAt->toLocalTime())
            : std::nullopt,
        .isReminder = entry.isReminder(),
        .systemTaskId = entry.systemTaskId,
        .isLocked = entry.isLocked
    };
}
