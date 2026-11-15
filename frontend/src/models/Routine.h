#ifndef FRONTEND_ROUTINE_H
#define FRONTEND_ROUTINE_H

#include <QString>

struct Routine {
    int id;
    int systemTaskId;
    int domainNodeId;
    QString name;
    QString recurrenceRuleSet;
    QString timezone;
    int estimatedDuration;
    bool isOptional;
    bool isActive;
};

struct SaveRoutine {
    QString name;
    QString recurrenceRuleSet;
    QString timezone;
    int estimatedDuration;
    bool isOptional;
    bool isActive;
};

#endif // FRONTEND_ROUTINE_H
