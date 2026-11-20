#ifndef FRONTEND_DOMAINVIEW_H
#define FRONTEND_DOMAINVIEW_H

#include "../api/ApiClient.h"

#include <QWidget>

#include <functional>

class QPoint;
class QModelIndex;
class QLabel;
class QComboBox;
class QPushButton;
class QTimer;
class QStandardItem;
class QStandardItemModel;
class DocumentEditor;
class SaveStatus;
class QListView;
class QTreeView;
class QHBoxLayout;
class QVBoxLayout;

class DomainView : public QWidget {
public:
    using NodeOpenHandler = std::function<void(int, const QString &)>;
    using BeforeNodeActionHandler = std::function<
        void(int, std::function<void()>)
    >;

    explicit DomainView(ApiClient &apiClient, QWidget *parent = nullptr);
    void saveBeforeClose(std::function<void()> continuation);
    void refreshTasks();
    void setNodeOpenHandler(NodeOpenHandler handler);
    void setBeforeNodeActionHandler(BeforeNodeActionHandler handler);
    void showNode(int nodeId);

private:
    void renderDomainTree(
        const std::vector<MetadataItem> &domains,
        const std::vector<DomainNode> &nodes
    );
    void showDomainNodeContextMenu(const QPoint &position);
    void showDomainNodeComments(int nodeId);
    void createTask(int nodeId);
    void editTask(int taskId);
    void showTaskComments(int taskId);
    QStandardItem *findDomainItem(int domainId);
    QStandardItem *findDomainNodeItem(int nodeId);
    void requestDocumentChange(int nodeId, bool hasDocument = false);
    void continueDocumentChange();
    void startSave();
    void finishPendingActions();
    void createFlashcard(int nodeId);
    void editFlashcard(int row);
    void showFlashcardComments(int flashcardId);
    void updateFlashcardCount(int nodeId, int count);
    void updateFlashcardView();
    void addAttachments(int nodeId);
    void renderAttachments(const std::vector<Attachment> &attachments);
    void updateAttachmentCount(int nodeId, int count);
    void updateFlashcardStatus(std::optional<int> statusId);
    void updateDocumentStatus(
        std::optional<int> completenessId,
        std::optional<int> verificationStatusId,
        std::optional<int> verificationLevelId
    );
    void renderSkills(const std::vector<Skill> &skills);
    void showSkillContextMenu(const QPoint &position);
    void showSkillComments(int skillId);
    void createSkill(int nodeId, std::optional<int> parentSkillId = std::nullopt);
    void editSkill(const QModelIndex &index);
    void updateSkillCount(int nodeId, int count);
    void renderTasks();
    void renderPursuits();
    void createPursuit(int nodeId);
    void editPursuit(const QModelIndex &index);
    void showPursuitComments(int pursuitId);
    void updatePursuitCount(int nodeId, int count);
    void updateCurrentTaskCount();
    void updateNavigationTreeHeight();
    void prepareNodeAction(int nodeId, std::function<void()> continuation);

    ApiClient &apiClient;
    QTreeView *treeView;
    QStandardItemModel *model;
    std::vector<DomainNode> domainNodes;
    QWidget *editorWidget;
    QVBoxLayout *editorLayout;
    QWidget *documentWidget;
    QWidget *documentMetadataWidget;
    QLabel *completenessLabel;
    SaveStatus *saveStatus;
    DocumentEditor *documentEditor;
    QLabel *flashcardSectionStatusLabel;
    QLabel *noFlashcardsLabel;
    QListView *flashcardListView;
    QStandardItemModel *flashcardModel;
    QLabel *noAttachmentsLabel;
    QListView *attachmentListView;
    QStandardItemModel *attachmentModel;
    QWidget *skillWidget;
    QLabel *noTasksLabel;
    QListView *taskListView;
    QStandardItemModel *taskModel;
    QLabel *noPursuitsLabel;
    QTreeView *pursuitTreeView;
    QStandardItemModel *pursuitModel;
    QLabel *noSkillsLabel;
    QTreeView *skillTreeView;
    QStandardItemModel *skillModel;
    std::vector<Skill> currentSkills;
    std::vector<Pursuit> currentPursuits;
    std::vector<Task> currentTasks;
    std::vector<Flashcard> currentFlashcards;
    QTimer *autosaveTimer;
    QTimer *maximumAutosaveTimer;
    int currentNodeId = -1;
    int currentDocumentNodeId = -1;
    int pendingDocumentNodeId = -1;
    bool pendingNodeHasDocument = false;
    bool documentChangePending = false;
    bool documentLoadPending = false;
    bool saveInProgress = false;
    bool commentIndicatorPressed = false;
    int savingDocumentNodeId = -1;
    QString savedContent;
    QString savingContent;
    std::function<void()> closeContinuation;
    NodeOpenHandler nodeOpenHandler;
    BeforeNodeActionHandler beforeNodeActionHandler;
    int initialNodeId = -1;
    bool nodeOnly = false;
};

#endif // FRONTEND_DOMAINVIEW_H
