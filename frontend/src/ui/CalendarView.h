#ifndef FRONTEND_CALENDARVIEW_H
#define FRONTEND_CALENDARVIEW_H

#include "../api/ApiClient.h"
#include "../models/CalendarEntry.h"
#include "../models/Task.h"

#include <QWidget>

#include <vector>

class QTreeWidget;
class QDateEdit;

class CalendarView : public QWidget {
public:
    explicit CalendarView(ApiClient &apiClient, QWidget *parent = nullptr);

private:
    void renderEntries();
    void openCreateDialog();
    void openEditDialog(const CalendarEntry& entry);
    void openTaskEditDialog(const Task& task);

    ApiClient &apiClient;
    QTreeWidget *tree;
    QDateEdit *fromDateEdit;
    QDateEdit *toDateEdit;
    std::vector<CalendarEntry> entries;
    std::vector<Task> tasks;
};

#endif // FRONTEND_CALENDARVIEW_H
