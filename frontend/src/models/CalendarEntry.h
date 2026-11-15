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
    std::optional<int> systemTaskId;

    bool isReminder() const {
        return !endAt.has_value();
    }
};

struct CreateCalendarEntry {
    QString name;
    QDateTime startAt;
    std::optional<QDateTime> endAt;
    bool isReminder;
    std::optional<int> systemTaskId;
};

#endif // FRONTEND_CALENDARENTRY_H
