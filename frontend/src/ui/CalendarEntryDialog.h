#ifndef FRONTEND_CALENDARENTRYDIALOG_H
#define FRONTEND_CALENDARENTRYDIALOG_H

#include "../models/CalendarEntry.h"

#include <QString>

#include <optional>

class QWidget;

std::optional<CreateCalendarEntry> showCalendarEntryDialog(
    QWidget *parent,
    const QString &title,
    const CreateCalendarEntry &initialEntry,
    const QString &acceptText,
    bool acceptOnOutsideClick
);

#endif // FRONTEND_CALENDARENTRYDIALOG_H
