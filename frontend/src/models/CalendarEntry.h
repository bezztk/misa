#ifndef FRONTEND_CALENDARENTRY_H
#define FRONTEND_CALENDARENTRY_H

#include <QDateTime>
#include <QString>

#include <optional>

struct CalendarEntry {
    int id;
    QString name;
    QDateTime startAt;
    std::optional<QDateTime> endAt{};
    std::optional<int> systemTaskId{};
    std::optional<int> routineExecutionId{};
    bool isLocked = false;

    bool isReminder() const {
        return !endAt.has_value();
    }
};

struct CreateCalendarEntry {
    QString name{};
    QDateTime startAt;
    std::optional<QDateTime> endAt{};
    bool isReminder;
    std::optional<int> systemTaskId{};
    bool isLocked = false;
};

#endif // FRONTEND_CALENDARENTRY_H
