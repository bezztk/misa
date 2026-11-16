#ifndef FRONTEND_ROUTINEDIALOG_H
#define FRONTEND_ROUTINEDIALOG_H

#include "../models/Routine.h"

#include <QString>

#include <optional>

class QWidget;

bool showRoutineDialog(
    QWidget *parent,
    const QString &title,
    SaveRoutine &routine,
    std::optional<Routine> existing = std::nullopt
);

#endif // FRONTEND_ROUTINEDIALOG_H
