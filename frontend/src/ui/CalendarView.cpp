#include "CalendarView.h"
#include "CalendarEntryDialog.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

void CalendarBridge::createEntryRequested() {
    emit createEntry();
}

void CalendarBridge::refreshRequested() {
    emit refresh();
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
    connect(bridge, &CalendarBridge::createEntry,
            this, &CalendarView::openCreateDialog);
    connect(bridge, &CalendarBridge::refresh, this, [this] {
        apiClient.generateRoutineExecutions();
    });

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
