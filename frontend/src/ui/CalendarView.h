#ifndef FRONTEND_CALENDARVIEW_H
#define FRONTEND_CALENDARVIEW_H

#include "../models/CalendarEntry.h"

#include <QDate>
#include <QWidget>

#include <vector>

class ApiClient;
class QLabel;
class QPoint;

class CalendarView final : public QWidget {
    Q_OBJECT

public:
    explicit CalendarView(ApiClient &apiClient, QWidget *parent = nullptr);
    void openCreateDialog();
    void refresh();

private:
    class DayGrid;

    void setDate(QDate date);
    void renderDay();
    void openEditDialog(int id);
    void showEntryContextMenu(int id, const QPoint &position);
    [[nodiscard]] const CalendarEntry *findEntry(int id) const;

    ApiClient &apiClient;
    QDate selectedDate;
    std::vector<CalendarEntry> entries;
    QLabel *dateLabel;
    DayGrid *grid;
};

#endif // FRONTEND_CALENDARVIEW_H
