#ifndef FRONTEND_VERSIONHISTORYDIALOG_H
#define FRONTEND_VERSIONHISTORYDIALOG_H

#include <QString>

class ApiClient;
class QWidget;

void showVersionHistoryDialog(
    QWidget *parent,
    ApiClient &apiClient,
    int entityId,
    const QString &title,
    bool idea = false
);

void showDocumentVersionHistoryDialog(
    QWidget *parent,
    ApiClient &apiClient,
    int nodeId,
    const QString &title
);

#endif // FRONTEND_VERSIONHISTORYDIALOG_H
