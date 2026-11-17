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
class QStackedWidget;
class QTextBrowser;
class QTreeView;
class QHBoxLayout;
class QVBoxLayout;

class DomainView : public QWidget {
public:
    explicit DomainView(ApiClient &apiClient, QWidget *parent = nullptr);
    void saveBeforeClose(std::function<void()> continuation);
    void refreshTasks();

private:
    void renderDomainTree(
        const std::vector<MetadataItem> &domains,
        const std::vector<DomainNode> &nodes
    );
    void showDomainNodeContextMenu(const QPoint &position);
    void createRoutine(int nodeId, const QString& nodeName);
    void editRoutine(const Routine& routine);
    QStandardItem *findDomainItem(int domainId);
    QStandardItem *findDomainNodeItem(int nodeId);
    void requestDocumentChange(int nodeId, bool hasDocument = false);
    void continueDocumentChange();
    void startSave();
    void finishPendingActions();
    void createFlashcard(int nodeId);
    void editFlashcard(int row);
    void updateFlashcardCount(int nodeId, int count);
    void updateFlashcardView();
    void updateFlashcardStatus(std::optional<int> statusId);
    void updateDocumentStatus(
        std::optional<int> completenessId,
        std::optional<int> verificationStatusId,
        std::optional<int> verificationLevelId
    );
    void renderSkills(const std::vector<Skill> &skills);
    void showSkillContextMenu(const QPoint &position);
    void createSkill(int nodeId, std::optional<int> parentSkillId = std::nullopt);
    void editSkill(const QModelIndex &index);
    void updateSkillCount(int nodeId, int count);
    void renderPursuits();
    void showPursuitContextMenu(const QPoint &position);
    void createPursuitTask(int pursuitId);
    void createPursuit(int nodeId);
    void editPursuit(const QModelIndex &index);
    void updatePursuitCount(int nodeId, int count);
    void updateCurrentTaskCount();
    void renderProgressions();
    void renderProgressionStages();
    void showProgressionContextMenu(const QPoint &position);
    void editProgression(int progressionId);
    void addProgressionStage(int progressionId);
    void assignPursuit(int stageId);
    void showAvailablePursuits(
        int stageId,
        const std::vector<ProgressionPursuit> &pursuits
    );

    ApiClient &apiClient;
    QTreeView *treeView;
    QStandardItemModel *model;
    QWidget *progressionWidget;
    QWidget *progressionContent;
    QTreeView *progressionTreeView;
    QStandardItemModel *progressionModel;
    QHBoxLayout *progressionStagesLayout;
    std::vector<Progression> progressions;
    std::vector<ProgressionPursuit> progressionPursuits;
    int currentProgressionId = -1;
    QWidget *editorWidget;
    QVBoxLayout *editorLayout;
    QWidget *documentWidget;
    QLabel *documentTitle;
    QWidget *documentMetadataWidget;
    QLabel *completenessLabel;
    QLabel *verificationLabel;
    SaveStatus *saveStatus;
    DocumentEditor *documentEditor;
    QStackedWidget *documentStack;
    QTextBrowser *documentPreview;
    QPushButton *toggleDocumentPreviewButton;
    QLabel *flashcardSectionStatusLabel;
    QLabel *noFlashcardsLabel;
    QListView *flashcardListView;
    QStandardItemModel *flashcardModel;
    QWidget *skillWidget;
    QLabel *noPursuitsLabel;
    QTreeView *pursuitTreeView;
    QStandardItemModel *pursuitModel;
    QLabel *noSkillsLabel;
    QTreeView *skillTreeView;
    QStandardItemModel *skillModel;
    std::vector<Skill> currentSkills;
    std::vector<Pursuit> currentPursuits;
    std::vector<Task> currentTasks;
    QTimer *autosaveTimer;
    int currentNodeId = -1;
    int currentDocumentNodeId = -1;
    int pendingDocumentNodeId = -1;
    bool pendingNodeHasDocument = false;
    bool documentChangePending = false;
    bool documentLoadPending = false;
    bool saveInProgress = false;
    int savingDocumentNodeId = -1;
    QString savedContent;
    QString savingContent;
    std::function<void()> closeContinuation;
};

#endif // FRONTEND_DOMAINVIEW_H
