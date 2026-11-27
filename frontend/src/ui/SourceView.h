#ifndef FRONTEND_SOURCEVIEW_H
#define FRONTEND_SOURCEVIEW_H

#include "../models/Source.h"

#include <vector>
#include <QWidget>

class ApiClient;
class QComboBox;
class QLineEdit;
class QListWidget;
class QPoint;

class SourceView final : public QWidget {
public:
    explicit SourceView(ApiClient &apiClient, QWidget *parent = nullptr);
    void openCreateDialog();

private:
    void rebuildList();
    void applyFilters();
    void showContextMenu(const QPoint &position);

    ApiClient &apiClient;
    QLineEdit *search;
    QComboBox *typeFilter;
    QListWidget *list;
    std::vector<Source> sources;
};

#endif // FRONTEND_SOURCEVIEW_H
