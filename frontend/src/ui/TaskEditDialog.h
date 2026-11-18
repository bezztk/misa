#ifndef FRONTEND_TASKEDITDIALOG_H
#define FRONTEND_TASKEDITDIALOG_H

class ApiClient;
class QWidget;
struct Task;

void showTaskEditDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const Task &task
);

#endif // FRONTEND_TASKEDITDIALOG_H
