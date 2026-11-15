#ifndef FRONTEND_CALENDARVIEW_H
#define FRONTEND_CALENDARVIEW_H

#include "../api/ApiClient.h"
#include "../models/CalendarEntry.h"

#include <QWidget>

#include <vector>

class QTreeWidget;

class CalendarView : public QWidget {
public:
    explicit CalendarView(QWidget *parent = nullptr);

private:
    void renderEntries();
    void openCreateDialog();

    ApiClient apiClient;
    QTreeWidget *tree;
    std::vector<CalendarEntry> entries;
};

#endif // FRONTEND_CALENDARVIEW_H
