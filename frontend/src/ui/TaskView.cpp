#include "TaskView.h"
#include "CalendarEntryDialog.h"
#include "CommentUi.h"
#include "DialogUtils.h"
#include "TaskEditDialog.h"
#include "VersionHistoryDialog.h"

#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QResizeEvent>
#include <QUrl>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineView>

#include <algorithm>

void TaskBridge::kanbanReady() {
    emit ready();
}

void TaskBridge::taskStateChangeRequested(const int id, const int stateId) {
    emit changeTaskState(id, stateId);
}

void TaskBridge::editTaskRequested(const int id) {
    emit editTask(id);
}

void TaskBridge::contextMenuRequested(const int id, const int x, const int y) {
    emit showContextMenu(id, x, y);
}

static QString metadataName(const MetadataItem &item) {
    return item.displayName.value_or(item.name);
}

static QString taskPriorityColor(const Metadata &metadata, const int id) {
    for (const MetadataItem &priority : metadata.systemTaskPriorities) {
        if (priority.id != id) {
            continue;
        }
        if (priority.name == "low") {
            return "#64b5f6";
        }
        if (priority.name == "high") {
            return "#ffb74d";
        }
        if (priority.name == "critical") {
            return "#ef5350";
        }
    }
    return "#a0a0a0";
}

TaskView::TaskView(ApiClient &client, QWidget *parent)
    : QWidget(parent),
      apiClient(client),
      webView(new QWebEngineView(this)),
      webChannel(new QWebChannel(this)),
      bridge(new TaskBridge(this)) {
    webView->setGeometry(rect());
    webChannel->registerObject(QStringLiteral("taskBridge"), bridge);
    webView->page()->setWebChannel(webChannel);

    connect(bridge, &TaskBridge::ready, this, [this] {
        kanbanReady = true;
        pushBoardData();
    });
    connect(bridge, &TaskBridge::changeTaskState, this,
            [this](const int id, const int stateId) {
                apiClient.updateTaskField(id, TaskField::StateId, stateId);
            });
    connect(bridge, &TaskBridge::editTask, this, [this](const int id) {
        const Task *task = taskById(id);
        if (task) {
            openEditDialog(*task);
        }
    });
    connect(bridge, &TaskBridge::showContextMenu,
            this, &TaskView::showTaskContextMenu);

    connect(&apiClient, &ApiClient::taskReceived, this,
            [this](const std::vector<Task> &receivedTasks) {
                tasks = receivedTasks;
                pushBoardData();
            });
    connect(&apiClient, &ApiClient::domainTreeReceived, this,
            [this](const std::vector<MetadataItem> &,
                   const std::vector<DomainNode> &receivedNodes) {
                domainNodes = receivedNodes;
                pushBoardData();
            });
    connect(&apiClient, &ApiClient::domainNodeCreated, this,
            [this](const DomainNode &node) {
                domainNodes.push_back(node);
                pushBoardData();
            });
    connect(&apiClient, &ApiClient::domainNodeFieldUpdated, this,
            [this](const int nodeId, const DomainNodeField field,
                   const QJsonValue &value) {
                if (field != DomainNodeField::Name) {
                    return;
                }
                const auto existing = std::find_if(
                    domainNodes.begin(), domainNodes.end(),
                    [nodeId](const DomainNode &item) { return item.id == nodeId; }
                );
                if (existing != domainNodes.end()) {
                    existing->name = value.toString();
                    pushBoardData();
                }
            });
    connect(&apiClient, &ApiClient::domainNodeDeleted, this,
            [this](const int nodeId) {
                std::erase_if(domainNodes, [nodeId](const DomainNode &node) {
                    return node.id == nodeId;
                });
                pushBoardData();
            });

    webView->setUrl(QUrl(QStringLiteral("qrc:/kanban/index.html")));
}

void TaskView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    webView->setGeometry(rect());
}

void TaskView::refreshTasks() {
    apiClient.getTasks();
}

void TaskView::refreshMetadata() {
    pushBoardData();
}

void TaskView::pushBoardData() {
    if (!kanbanReady) {
        return;
    }

    QJsonArray taskItems;
    for (const Task &task : tasks) {
        taskItems.append(QJsonObject{
            {"id", task.id},
            {"label", task.name},
            {"column", task.stateId},
            {"groupId", task.groupId.has_value()
                ? QJsonValue(*task.groupId) : QJsonValue(QJsonValue::Null)},
            {"domainNodeId", task.domainNodeId.has_value()
                ? QJsonValue(*task.domainNodeId) : QJsonValue(QJsonValue::Null)},
            {"priorityColor", taskPriorityColor(apiClient.metadata(), task.priorityId)},
            {"commentCount", static_cast<int>(task.comments.size())}
        });
    }

    QJsonArray states;
    for (const MetadataItem &state : apiClient.metadata().systemTaskStates) {
        states.append(QJsonObject{
            {"id", state.id},
            {"label", metadataName(state)}
        });
    }

    QJsonArray groups;
    for (const MetadataItem &group : apiClient.metadata().systemTaskGroups) {
        groups.append(QJsonObject{
            {"id", group.id},
            {"name", metadataName(group)}
        });
    }

    QJsonArray nodes;
    for (const DomainNode &node : domainNodes) {
        nodes.append(QJsonObject{{"id", node.id}, {"name", node.name}});
    }

    const QJsonObject board{
        {"tasks", taskItems},
        {"states", states},
        {"groups", groups},
        {"domainNodes", nodes}
    };
    emit bridge->boardChanged(QString::fromUtf8(
        QJsonDocument(board).toJson(QJsonDocument::Compact)
    ));
}

Task *TaskView::taskById(const int id) {
    const auto task = std::find_if(tasks.begin(), tasks.end(),
                                   [id](const Task &item) { return item.id == id; });
    return task == tasks.end() ? nullptr : &*task;
}

void TaskView::openCreateDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Aufgabe erstellen");
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(&dialog);
    layout->addRow("Name", nameEdit);

    auto *groupEdit = new QComboBox(&dialog);
    for (const MetadataItem &group : apiClient.metadata().systemTaskGroups) {
        groupEdit->addItem(group.name, group.id);
    }
    layout->addRow("Gruppe", groupEdit);

    auto *priorityEdit = new QComboBox(&dialog);
    for (const MetadataItem &priority : apiClient.metadata().systemTaskPriorities) {
        priorityEdit->addItem(metadataName(priority), priority.id);
    }
    layout->addRow("Priorität", priorityEdit);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog
    );
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    buttons->button(QDialogButtonBox::Save)->setText("Erstellen");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    layout->addRow(buttons);
    prepareDialog(dialog);

    if (execDialog(dialog) != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return;
    }

    apiClient.createTask(CreateTask{
        .name = nameEdit->text().trimmed(),
        .groupId = groupEdit->currentData().toInt(),
        .priorityId = priorityEdit->currentData().toInt(),
        .domainNodeId = std::nullopt
    });
}

void TaskView::openEditDialog(const Task &task) {
    showTaskEditDialog(this, apiClient, task);
}

void TaskView::openScheduleDialog(const Task &task) {
    const QDateTime initialStart = QDateTime::currentDateTime();
    const auto entry = showCalendarEntryDialog(this, "Aufgabe einplanen", {
        .name = task.name,
        .startAt = initialStart,
        .endAt = initialStart.addSecs(3600),
        .isReminder = true,
        .systemTaskId = task.id
    }, "Erstellen", false);
    if (entry) {
        apiClient.createCalendarEntry(*entry);
    }
}

void TaskView::showTaskContextMenu(const int id, const int x, const int y) {
    const Task *task = taskById(id);
    if (!task) {
        return;
    }

    QMenu menu(webView);
    QAction *editAction = menu.addAction(
        QIcon(":/assets/icons/edit.png"),
        "Bearbeiten"
    );
    QAction *scheduleAction = menu.addAction(
        QIcon(":/assets/icons/clock.png"),
        "Einplanen"
    );
    QAction *commentsAction = menu.addAction(
        QIcon(":/assets/icons/speech-bubble.png"),
        task->comments.empty()
            ? "Kommentare anzeigen"
            : "Kommentare anzeigen (" + QString::number(task->comments.size()) + ")"
    );
    menu.addSeparator();
    QAction *historyAction = menu.addAction(
        QIcon(":/assets/icons/version_history.png"),
        "Versionshistorie"
    );
    menu.addSeparator();
    QAction *deleteAction = menu.addAction(
        QIcon(":/assets/icons/delete.png"),
        "Löschen"
    );

    QAction *selectedAction = menu.exec(webView->mapToGlobal(QPoint(x, y)));
    if (selectedAction == editAction) {
        openEditDialog(*task);
    } else if (selectedAction == scheduleAction) {
        openScheduleDialog(*task);
    } else if (selectedAction == commentsAction) {
        Task *editableTask = taskById(id);
        showCommentsDialog(
            webView,
            apiClient,
            editableTask->id,
            "Kommentare – " + editableTask->name,
            editableTask->comments
        );
        pushBoardData();
    } else if (selectedAction == historyAction) {
        showVersionHistoryDialog(
            webView,
            apiClient,
            task->id,
            "Versionshistorie – " + task->name
        );
    } else if (selectedAction == deleteAction) {
        apiClient.deleteTask(task->id);
    }
}
