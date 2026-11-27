#ifndef FRONTEND_TASKEDITDIALOG_H
#define FRONTEND_TASKEDITDIALOG_H

#include "../models/Context.h"
#include "../models/Task.h"

class ApiClient;
class QWidget;

void showTaskCreateDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const std::vector<Context> &contexts,
    const std::vector<int> &selectedIds,
    std::optional<int> domainNodeId = std::nullopt
);

void showTaskEditDialog(
    QWidget *parent,
    ApiClient &apiClient,
    Task task,
    const std::vector<Context> &contexts = {}
);

#endif // FRONTEND_TASKEDITDIALOG_H
