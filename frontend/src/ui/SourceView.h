#ifndef FRONTEND_SOURCEVIEW_H
#define FRONTEND_SOURCEVIEW_H

#include "../models/Source.h"

#include <vector>
#include <QWidget>

class ApiClient;
class QComboBox;
class QLineEdit;
class QTreeWidget;
class QPoint;

class SourceView final : public QWidget {
public:
    explicit SourceView(ApiClient &apiClient, QWidget *parent = nullptr);
    void openCreateDialog();
    void openCreateCategoryDialog();

private:
    void refresh();
    void rebuildTree();
    void applyFilters();
    void showContextMenu(const QPoint &position);
    void showSourceMenu(const Source &source, const QPoint &position);
    void showCategoryMenu(int id, const QString &name, const QPoint &position);
    void openCategoryDialog(const Source &source);

    ApiClient &apiClient;
    QLineEdit *search;
    QComboBox *typeFilter;
    QComboBox *categoryFilter;
    QTreeWidget *tree;
    std::vector<Source> sources;
    std::vector<SourceCategory> categories;
};

#endif // FRONTEND_SOURCEVIEW_H
