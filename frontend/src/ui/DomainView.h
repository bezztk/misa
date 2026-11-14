#ifndef FRONTEND_DOMAINVIEW_H
#define FRONTEND_DOMAINVIEW_H

#include "../api/ApiClient.h"

#include <QWidget>

#include <functional>

class QPoint;
class QLabel;
class QComboBox;
class QPushButton;
class QTimer;
class QStandardItem;
class QStandardItemModel;
class QPlainTextEdit;
class QResizeEvent;
class QTableView;
class QTreeView;
class QVBoxLayout;

class DomainView : public QWidget {
public:
    explicit DomainView(QWidget *parent = nullptr);
    void saveBeforeClose(std::function<void()> continuation);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void renderDomainTree(
        const std::vector<MetadataItem> &domains,
        const std::vector<DomainNode> &nodes
    );
    void showDomainNodeContextMenu(const QPoint &position);
    QStandardItem *findDomainItem(int domainId);
    QStandardItem *findDomainNodeItem(int nodeId);
    void requestDocumentChange(int nodeId, bool hasDocument = false);
    void continueDocumentChange();
    void startSave();
    void finishPendingActions();
    void createFlashcard();
    void editFlashcard(int row);
    void updateFlashcardCount(int nodeId, int count);
    void updateFlashcardView();

    ApiClient apiClient;
    QTreeView *treeView;
    QStandardItemModel *model;
    QWidget *editorWidget;
    QVBoxLayout *editorLayout;
    QWidget *documentWidget;
    QLabel *documentTitle;
    QWidget *documentMetadataWidget;
    QLabel *completenessLabel;
    QLabel *flashcardStatusLabel;
    QLabel *verificationStatusLabel;
    QLabel *verificationLevelLabel;
    QWidget *saveStatus;
    QPlainTextEdit *documentEditor;
    QWidget *flashcardWidget;
    QLabel *flashcardTitle;
    QPushButton *toggleFlashcardsButton;
    QTableView *flashcardTable;
    QStandardItemModel *flashcardModel;
    QTimer *autosaveTimer;
    int currentNodeId = -1;
    int currentDocumentNodeId = -1;
    int pendingDocumentNodeId = -1;
    bool pendingNodeHasDocument = false;
    bool documentChangePending = false;
    bool documentLoadPending = false;
    bool flashcardsExpanded = true;
    bool saveInProgress = false;
    QString savedContent;
    QString savingContent;
    std::function<void()> closeContinuation;
};

#endif // FRONTEND_DOMAINVIEW_H
