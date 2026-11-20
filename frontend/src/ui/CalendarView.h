#ifndef FRONTEND_CALENDARVIEW_H
#define FRONTEND_CALENDARVIEW_H

#include "../api/ApiClient.h"
#include "../models/CalendarEntry.h"

#include <QObject>
#include <QString>
#include <QWidget>

#include <optional>
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
    void timeRangeSelected(const QString &start, const QString &end);
    void editEntryRequested(int id);
    void contextMenuRequested(int id, int x, int y);
    void entryChangeRequested(int id, const QString &start, const QString &end);

signals:
    void ready();
    void visibleRangeChanged(const QString &start, const QString &end);
    void createEntry();
    void refresh();
    void createEntryForRange(const QString &start, const QString &end);
    void editEntry(int id);
    void showContextMenu(int id, int x, int y);
    void changeEntry(int id, const QString &start, const QString &end);
    void entriesChanged(const QString &entriesJson);
    void entryChangeFailed(int id);
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
    void openCreateDialog(const QDateTime &start, const QDateTime &end);
    void openEditDialog(int id);
    void showEntryContextMenu(int id, int x, int y);
    void updateEntryTime(int id, const QString &start, const QString &end);
    [[nodiscard]] const CalendarEntry *findEntry(int id) const;
    static CreateCalendarEntry editableEntry(const CalendarEntry &entry);

    ApiClient &apiClient;
    QWebEngineView *webView;
    QWebChannel *webChannel;
    CalendarBridge *bridge;
    std::vector<CalendarEntry> entries;
    bool calendarReady = false;
};

#endif // FRONTEND_CALENDARVIEW_H
