#ifndef FRONTEND_CALENDARENTRYDIALOG_H
#define FRONTEND_CALENDARENTRYDIALOG_H

#include "../models/CalendarEntry.h"

#include <QString>

#include <optional>

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

#endif // FRONTEND_CALENDARENTRYDIALOG_H
