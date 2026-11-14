#ifndef FRONTEND_DOMAINVIEW_H
#define FRONTEND_DOMAINVIEW_H

#include "../api/ApiClient.h"

#include <QWidget>

class QPoint;
class QLabel;
class QPushButton;
class QStandardItem;
class QStandardItemModel;
class QPlainTextEdit;
class QTreeView;

class DomainView : public QWidget {
public:
    explicit DomainView(QWidget *parent = nullptr);

private:
    void renderDomainTree(
        const std::vector<MetadataItem> &domains,
        const std::vector<DomainNode> &nodes
    );
    void showDomainNodeContextMenu(const QPoint &position);
    QStandardItem *findDomainItem(int domainId);
    QStandardItem *findDomainNodeItem(int nodeId);

    ApiClient apiClient;
    QTreeView *treeView;
    QStandardItemModel *model;
    QWidget *editorWidget;
    QLabel *documentTitle;
    QPlainTextEdit *documentEditor;
    QPushButton *saveButton;
    int currentDocumentNodeId = -1;
};

#endif // FRONTEND_DOMAINVIEW_H
