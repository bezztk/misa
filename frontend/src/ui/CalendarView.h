#ifndef FRONTEND_CALENDARVIEW_H
#define FRONTEND_CALENDARVIEW_H

#include "../models/CalendarEntry.h"
#include "../models/Context.h"
#include "../models/Task.h"

#include <QDate>
#include <QWidget>

#include <vector>

class ApiClient;
class QAbstractButton;
class QButtonGroup;
class QLabel;
class QPoint;
class QScrollArea;
class QStackedWidget;

class CalendarView final : public QWidget {
    Q_OBJECT

public:
    explicit CalendarView(ApiClient &apiClient, QWidget *parent = nullptr);
    void openCreateDialog();
    void refresh();

signals:
    void openTaskRequested(int id);
    void openDomainNodeRequested(int id, const QString &title);

private:
    enum class ViewMode { Day, Week, Month };
    class TimeGrid;

    void showDate(QDate date, ViewMode mode);
    void navigate(int step);
    void renderCalendar();
    void renderMonth();
    QAbstractButton *createEntryButton(const CalendarEntry &entry, QWidget *parent);
    void openEditDialog(int id);
    void showEntryContextMenu(int id, const QPoint &position);
    [[nodiscard]] const CalendarEntry *findEntry(int id) const;
    [[nodiscard]] const Task *findLinkedTask(const CalendarEntry &entry) const;

    ApiClient &apiClient;
    QDate today = QDate::currentDate();
    QDate selectedDate;
    ViewMode viewMode = ViewMode::Day;
    std::vector<CalendarEntry> entries;
    std::vector<Task> tasks;
    std::vector<Context> contexts;
    QLabel *dateLabel;
    QButtonGroup *viewButtons;
    QStackedWidget *pages;
    TimeGrid *grid;
    QScrollArea *monthScroll;
};

#endif // FRONTEND_CALENDARVIEW_H
