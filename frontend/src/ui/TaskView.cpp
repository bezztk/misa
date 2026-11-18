#include "TaskView.h"
#include "CalendarEntryDialog.h"
#include "DialogUtils.h"
#include "TaskEditDialog.h"
#include "VersionHistoryDialog.h"

#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>

static constexpr int NAVIGATION_TYPE_ROLE = Qt::UserRole;
static constexpr int NAVIGATION_ID_ROLE = Qt::UserRole + 1;
static constexpr int TASK_ID_ROLE = Qt::UserRole;

static constexpr int NAVIGATION_ALL = 0;
static constexpr int NAVIGATION_GROUP = 1;
static constexpr int NAVIGATION_DOMAIN_NODE = 2;

static QString metadataName(const std::vector<MetadataItem> &items, const int id) {
    for (const MetadataItem &item : items) {
        if (item.id == id) {
            return item.displayName.value_or(item.name);
        }
    }
    return QString::number(id);
}

static QString taskStateName(const Metadata &metadata, const int id) {
    return metadataName(metadata.systemTaskStates, id);
}

static QColor taskPriorityColor(const Metadata &metadata, const int id) {
    for (const MetadataItem &priority : metadata.systemTaskPriorities) {
        if (priority.id != id) {
            continue;
        }
        if (priority.name == "low") {
            return QColor("#64B5F6");
        }
        if (priority.name == "high") {
            return QColor("#FFB74D");
        }
        if (priority.name == "critical") {
            return QColor("#EF5350");
        }
    }
    return QColor("#E0E0E0");
}

static bool isActive(const Task &task) {
    return task.stateId == 1 || task.stateId == 2;
}

TaskView::TaskView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *content = new QWidget(this);
    content->setMaximumWidth(500);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(24, 20, 24, 20);
    contentLayout->setSpacing(14);

    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel("Aufgaben", content);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    title->setFont(titleFont);

    viewSelector = new QComboBox(content);
    viewSelector->setMinimumWidth(220);
    viewSelector->setMaximumWidth(320);
    viewSelector->setMinimumContentsLength(24);
    viewSelector->setSizeAdjustPolicy(
        QComboBox::AdjustToMinimumContentsLengthWithIcon
    );

    auto *createButton = new QPushButton("Neue Aufgabe", content);
    connect(createButton, &QPushButton::clicked, this, [this] { openCreateDialog(); });
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(createButton);
    contentLayout->addLayout(headerLayout);
    contentLayout->addWidget(viewSelector, 0, Qt::AlignLeft);

    taskList = new QTreeWidget(content);
    taskList->setColumnCount(2);
    taskList->setHeaderLabels({"Aufgabe", "Status"});
    taskList->setRootIsDecorated(false);
    taskList->setAlternatingRowColors(true);
    taskList->setUniformRowHeights(true);
    taskList->header()->setStretchLastSection(false);
    taskList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    taskList->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    taskList->setContextMenuPolicy(Qt::CustomContextMenu);
    contentLayout->addWidget(taskList, 1);
    layout->addStretch();
    layout->addWidget(content, 1);
    layout->addStretch();

    connect(viewSelector, &QComboBox::currentIndexChanged, this,
            [this](const int index) {
                if (index < 0) {
                    return;
                }
                selectedNavigationType = viewSelector->itemData(
                    index, NAVIGATION_TYPE_ROLE
                ).toInt();
                selectedNavigationId = viewSelector->itemData(
                    index, NAVIGATION_ID_ROLE
                ).toInt();
                renderTasks();
            });
    connect(taskList, &QWidget::customContextMenuRequested, this,
            [this](const QPoint &position) {
                QTreeWidgetItem *item = taskList->itemAt(position);
                if (!item) {
                    return;
                }
                const Task *task = taskById(item->data(0, TASK_ID_ROLE).toInt());
                if (!task) {
                    return;
                }
                QMenu menu(taskList);
                QAction *editAction = menu.addAction(
                    QIcon(":/assets/icons/edit.png"),
                    "Bearbeiten"
                );
                QAction *scheduleAction = menu.addAction(
                    QIcon(":/assets/icons/clock.png"),
                    "Einplanen"
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
                QAction *selectedAction = menu.exec(
                    taskList->viewport()->mapToGlobal(position)
                );
                if (selectedAction == editAction) {
                    openEditDialog(*task);
                } else if (selectedAction == scheduleAction) {
                    openScheduleDialog(*task);
                } else if (selectedAction == deleteAction) {
                    apiClient.deleteTask(task->id);
                } else if (selectedAction == historyAction) {
                    showVersionHistoryDialog(
                        taskList,
                        apiClient,
                        task->id,
                        "Versionshistorie – " + task->name
                    );
                }
            });

    connect(&apiClient, &ApiClient::taskReceived, this,
            [this](const std::vector<Task> &receivedTasks) {
                tasks = receivedTasks;
                renderViewSelector();
                renderTasks();
            });
    connect(&apiClient, &ApiClient::domainTreeReceived, this,
            [this](const std::vector<MetadataItem> &,
                   const std::vector<DomainNode> &receivedNodes) {
                domainNodes = receivedNodes;
                renderViewSelector();
                renderTasks();
            });
    connect(&apiClient, &ApiClient::domainNodeCreated, this,
            [this](const DomainNode &node) { domainNodes.push_back(node); });
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
                    renderViewSelector();
                }
            });
    connect(&apiClient, &ApiClient::domainNodeDeleted, this,
            [this](const int nodeId) {
                std::erase_if(domainNodes, [nodeId](const DomainNode &node) {
                    return node.id == nodeId;
                });
            });
}

void TaskView::refreshTasks() {
    apiClient.getTasks();
}

void TaskView::refreshMetadata() {
    renderViewSelector();
    renderTasks();
}

void TaskView::renderViewSelector() {
    const int previousType = selectedNavigationType;
    const int previousId = selectedNavigationId;
    viewSelector->blockSignals(true);
    viewSelector->clear();

    const auto countFor = [this](const int type, const int id) {
        return static_cast<int>(std::count_if(tasks.begin(), tasks.end(),
            [type, id](const Task &task) {
                if (!isActive(task)) {
                    return false;
                }
                if (type == NAVIGATION_GROUP) {
                    return task.groupId == id;
                }
                if (type == NAVIGATION_DOMAIN_NODE) {
                    return task.domainNodeId == id;
                }
                return true;
            }));
    };
    int selectedIndex = -1;
    const auto addEntry = [this, &countFor, previousType, previousId, &selectedIndex](
                              const QString &name, const int type, const int id) {
        const int index = viewSelector->count();
        viewSelector->addItem(
            name + " (" + QString::number(countFor(type, id)) + ")"
        );
        viewSelector->setItemData(index, type, NAVIGATION_TYPE_ROLE);
        viewSelector->setItemData(index, id, NAVIGATION_ID_ROLE);
        if (type == previousType && id == previousId) {
            selectedIndex = index;
        }
    };
    const auto addHeading = [this](const QString &text) {
        viewSelector->addItem(text);
        auto *model = static_cast<QStandardItemModel *>(viewSelector->model());
        QStandardItem *item = model->item(viewSelector->count() - 1);
        QFont font = item->font();
        font.setBold(true);
        item->setFont(font);
        item->setFlags(Qt::NoItemFlags);
    };

    addEntry("Alle Aufgaben", NAVIGATION_ALL, 0);
    addHeading("Aufgabengruppen");
    for (const MetadataItem &group : apiClient.metadata().systemTaskGroups) {
        addEntry(group.name, NAVIGATION_GROUP, group.id);
    }
    addHeading("Domain Nodes");
    for (const DomainNode &node : domainNodes) {
        if (countFor(NAVIGATION_DOMAIN_NODE, node.id) > 0) {
            addEntry(node.name, NAVIGATION_DOMAIN_NODE, node.id);
        }
    }

    if (selectedIndex < 0) {
        selectedNavigationType = NAVIGATION_ALL;
        selectedNavigationId = 0;
        selectedIndex = 0;
    }
    viewSelector->setCurrentIndex(selectedIndex);
    viewSelector->blockSignals(false);
}

void TaskView::renderTasks() {
    taskList->clear();

    const auto matchesSelection = [this](const Task &task) {
        if (selectedNavigationType == NAVIGATION_GROUP) {
            return task.groupId == selectedNavigationId;
        }
        if (selectedNavigationType == NAVIGATION_DOMAIN_NODE) {
            return task.domainNodeId == selectedNavigationId;
        }
        return true;
    };
    const auto addTask = [this](const Task &task, const bool closed) {
        auto *item = new QTreeWidgetItem(taskList);
        item->setData(0, TASK_ID_ROLE, task.id);
        item->setText(0, task.name);
        item->setText(1, taskStateName(apiClient.metadata(), task.stateId));
        item->setForeground(0, taskPriorityColor(apiClient.metadata(), task.priorityId));
        if (closed) {
            for (int column = 0; column < taskList->columnCount(); ++column) {
                item->setForeground(column, QColor("#969696"));
            }
        }
    };

    bool hasActive = false;
    bool hasClosed = false;
    for (const Task &task : tasks) {
        if (matchesSelection(task) && isActive(task)) {
            addTask(task, false);
            hasActive = true;
        }
    }

    for (const Task &task : tasks) {
        if (matchesSelection(task) && !isActive(task)) {
            if (!hasClosed) {
                auto *separator = new QTreeWidgetItem(taskList);
                separator->setText(0, "Abgeschlossen und abgebrochen");
                QFont font = separator->font(0);
                font.setBold(true);
                separator->setFont(0, font);
                separator->setForeground(0, QColor("#969696"));
                separator->setFlags(Qt::NoItemFlags);
                separator->setFirstColumnSpanned(true);
                hasClosed = true;
            }
            addTask(task, true);
        }
    }

    if (!hasActive && !hasClosed) {
        auto *empty = new QTreeWidgetItem(taskList);
        empty->setText(0, "Keine Aufgaben in dieser Ansicht.");
        empty->setForeground(0, QColor("#969696"));
        empty->setFlags(Qt::NoItemFlags);
        empty->setFirstColumnSpanned(true);
    }
}

const Task *TaskView::taskById(const int id) const {
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
        priorityEdit->addItem(priority.displayName.value_or(priority.name), priority.id);
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
