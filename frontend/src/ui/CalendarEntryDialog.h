#ifndef FRONTEND_CALENDARENTRYDIALOG_H
#define FRONTEND_CALENDARENTRYDIALOG_H

#include "../models/CalendarEntry.h"

#include <QString>

#include <optional>

class ApiClient;
class QWidget;
struct Task;

std::optional<CreateCalendarEntry> showCalendarEntryDialog(
    QWidget *parent,
    const QString &title,
    const CreateCalendarEntry &initialEntry,
    const QString &acceptText,
    bool acceptOnOutsideClick
);

std::optional<CreateCalendarEntry> showTaskScheduleDialog(QWidget *parent, const Task &task, bool isReminder);
void showTaskPlanningDialog(QWidget *parent, ApiClient &apiClient, const Task &task);
CreateCalendarEntry editableCalendarEntry(const CalendarEntry &entry);
void editCalendarEntry(QWidget *parent, ApiClient &apiClient, CalendarEntry entry);

#endif // FRONTEND_CALENDARENTRYDIALOG_H
