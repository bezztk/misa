#ifndef FRONTEND_DOMAINVIEW_H
#define FRONTEND_DOMAINVIEW_H

#include "../api/ApiClient.h"

#include <QWidget>

#include <functional>

class QPoint;
class QLabel;
class QPushButton;
class QTimer;
class QStandardItem;
class QStandardItemModel;
class QPlainTextEdit;
class QTreeView;

class DomainView : public QWidget {
public:
    explicit DomainView(QWidget *parent = nullptr);
    void saveBeforeClose(std::function<void()> continuation);

private:
    void renderDomainTree(
        const std::vector<MetadataItem> &domains,
        const std::vector<DomainNode> &nodes
    );
    void showDomainNodeContextMenu(const QPoint &position);
    QStandardItem *findDomainItem(int domainId);
    QStandardItem *findDomainNodeItem(int nodeId);
    void requestDocumentChange(int nodeId);
    void continueDocumentChange();
    void startSave();
    void finishPendingActions();

    ApiClient apiClient;
    QTreeView *treeView;
    QStandardItemModel *model;
    QWidget *editorWidget;
    QLabel *documentTitle;
    QWidget *saveStatus;
    QPlainTextEdit *documentEditor;
    QTimer *autosaveTimer;
    int currentDocumentNodeId = -1;
    int pendingDocumentNodeId = -1;
    bool documentChangePending = false;
    bool documentLoadPending = false;
    bool saveInProgress = false;
    QString savedContent;
    QString savingContent;
    std::function<void()> closeContinuation;
};

#endif // FRONTEND_DOMAINVIEW_H
