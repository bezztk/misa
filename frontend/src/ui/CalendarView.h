#ifndef FRONTEND_CALENDARVIEW_H
#define FRONTEND_CALENDARVIEW_H

#include "../api/ApiClient.h"
#include "../models/CalendarEntry.h"

#include <QObject>
#include <QString>
#include <QWidget>

#include <vector>

class QResizeEvent;
class QWebChannel;
class QWebEngineView;

class CalendarBridge final : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

public slots:
    void calendarReady();
    void rangeChanged(const QString &start, const QString &end);
    void createEntryRequested();
    void refreshRequested();

signals:
    void ready();
    void visibleRangeChanged(const QString &start, const QString &end);
    void createEntry();
    void refresh();
    void entriesChanged(const QString &entriesJson);
    void resizeRequested();
};

class CalendarView final : public QWidget {
    Q_OBJECT

public:
    explicit CalendarView(ApiClient &apiClient, QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void pushEntries();
    void openCreateDialog();

    ApiClient &apiClient;
    QWebEngineView *webView;
    QWebChannel *webChannel;
    CalendarBridge *bridge;
    std::vector<CalendarEntry> entries;
    bool calendarReady = false;
};

#endif // FRONTEND_CALENDARVIEW_H
