#ifndef FRONTEND_TASKPRIORITY_H
#define FRONTEND_TASKPRIORITY_H

#include "../models/Metadata.h"

#include <algorithm>

inline constexpr auto importantTaskPriorityColor = "#DEA760";

inline const MetadataItem *importantTaskPriority(const Metadata &metadata, const int id) {
    const auto &priorities = metadata.systemTaskPriorities;
    const auto priority = std::ranges::find(priorities, id, &MetadataItem::id);
    if (priority != priorities.end() && (priority->name == "high" || priority->name == "critical")) {
        return &*priority;
    }
    return nullptr;
}

#endif // FRONTEND_TASKPRIORITY_H
