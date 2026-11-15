#ifndef FRONTEND_CALENDARENTRY_H
#define FRONTEND_CALENDARENTRY_H

#include <QDateTime>
#include <QString>

#include <optional>

struct CalendarEntry {
    int id;
    QString name;
    QDateTime startAt;
    std::optional<QDateTime> endAt;

    bool isReminder() const {
        return !endAt.has_value();
    }
};

struct CreateCalendarEntry {
    QString name;
    QDateTime startAt;
    std::optional<QDateTime> endAt;
    bool isReminder;
};

#endif // FRONTEND_CALENDARENTRY_H
