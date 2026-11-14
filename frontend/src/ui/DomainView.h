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
class QPlainTextEdit;
class QResizeEvent;
class QTableView;
class QTreeView;
class QHBoxLayout;
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
    void renderSkills(const std::vector<Skill> &skills);
    void showSkillContextMenu(const QPoint &position);
    void createSkill(std::optional<int> parentSkillId = std::nullopt);
    void editSkill(const QModelIndex &index);
    void updateSkillCount(int nodeId, int count);
    void renderPursuits();
    void createPursuit();
    void editPursuit(const QModelIndex &index);
    void updatePursuitCount(int nodeId, int count);
    void renderProgressions();
    void renderProgressionStages();
    void showProgressionContextMenu(const QPoint &position);
    void editProgression(int progressionId);
    void addProgressionStage(int progressionId);
    void assignPursuit(int stageId);

    ApiClient apiClient;
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
    QWidget *skillWidget;
    QLabel *noPursuitsLabel;
    QPushButton *addPursuitButton;
    QTreeView *pursuitTreeView;
    QStandardItemModel *pursuitModel;
    QLabel *noSkillsLabel;
    QPushButton *addSkillButton;
    QTreeView *skillTreeView;
    QStandardItemModel *skillModel;
    std::vector<Skill> currentSkills;
    std::vector<Pursuit> currentPursuits;
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
