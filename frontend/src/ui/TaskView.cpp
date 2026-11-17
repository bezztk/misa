#include "TaskView.h"
#include "CalendarEntryDialog.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>

static constexpr int NAVIGATION_TYPE_ROLE = Qt::UserRole;
static constexpr int NAVIGATION_ID_ROLE = Qt::UserRole + 1;
static constexpr int TASK_ID_ROLE = Qt::UserRole;

static constexpr int NAVIGATION_ALL = 0;
static constexpr int NAVIGATION_GROUP = 1;
static constexpr int NAVIGATION_PURSUIT = 2;

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

static QString taskPriorityName(const Metadata &metadata, const int id) {
    return metadataName(metadata.systemTaskPriorities, id);
}

static bool isActive(const Task &task) {
    return task.stateId == 1 || task.stateId == 2;
}

TaskView::TaskView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *sidebar = new QWidget(this);
    sidebar->setFixedWidth(250);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(16, 20, 12, 20);
    sidebarLayout->setSpacing(10);

    auto *sidebarTitle = new QLabel("Aufgaben", sidebar);
    QFont sidebarTitleFont = sidebarTitle->font();
    sidebarTitleFont.setBold(true);
    sidebarTitleFont.setPointSize(sidebarTitleFont.pointSize() + 2);
    sidebarTitle->setFont(sidebarTitleFont);
    sidebarLayout->addWidget(sidebarTitle);

    navigation = new QListWidget(sidebar);
    navigation->setFrameShape(QFrame::NoFrame);
    sidebarLayout->addWidget(navigation, 1);
    layout->addWidget(sidebar);

    auto *separator = new QFrame(this);
    separator->setFixedWidth(1);
    separator->setStyleSheet("background: rgba(255, 255, 255, 0.20); border: none;");
    layout->addWidget(separator);

    auto *content = new QWidget(this);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(24, 20, 24, 20);
    contentLayout->setSpacing(14);

    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel("Alle Aufgaben", content);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    title->setFont(titleFont);

    auto *createButton = new QPushButton("Neue Aufgabe", content);
    connect(createButton, &QPushButton::clicked, this, [this] { openCreateDialog(); });
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(createButton);
    contentLayout->addLayout(headerLayout);

    taskList = new QTreeWidget(content);
    taskList->setColumnCount(3);
    taskList->setHeaderLabels({"Aufgabe", "Status", "Priorität"});
    taskList->setRootIsDecorated(false);
    taskList->setAlternatingRowColors(true);
    taskList->setUniformRowHeights(true);
    taskList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    taskList->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    taskList->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    taskList->setContextMenuPolicy(Qt::CustomContextMenu);
    contentLayout->addWidget(taskList, 1);
    layout->addWidget(content, 1);

    connect(navigation, &QListWidget::currentItemChanged, this,
            [this, title](QListWidgetItem *current) {
                if (!current) {
                    return;
                }
                selectedNavigationType = current->data(NAVIGATION_TYPE_ROLE).toInt();
                selectedNavigationId = current->data(NAVIGATION_ID_ROLE).toInt();
                QString text = current->text();
                text.remove(QRegularExpression(" \\([0-9]+\\)$"));
                title->setText(text);
                renderTasks();
            });
    connect(taskList, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem *item) {
                if (const Task *task = taskById(item->data(0, TASK_ID_ROLE).toInt())) {
                    openEditDialog(*task);
                }
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
                QAction *scheduleAction = menu.addAction("Einplanen");
                if (menu.exec(taskList->viewport()->mapToGlobal(position)) == scheduleAction) {
                    openScheduleDialog(*task);
                }
            });

    connect(&apiClient, &ApiClient::taskReceived, this,
            [this](const std::vector<Task> &receivedTasks) {
                tasks = receivedTasks;
                renderNavigation();
                renderTasks();
            });
    connect(&apiClient, &ApiClient::taskPursuitsReceived, this,
            [this](const std::vector<TaskPursuit> &receivedPursuits) {
                pursuits = receivedPursuits;
                renderNavigation();
                renderTasks();
            });
}

void TaskView::refreshTasks() {
    apiClient.getTasks();
    apiClient.getTaskPursuits();
}

void TaskView::refreshMetadata() {
    renderNavigation();
    renderTasks();
}

void TaskView::renderNavigation() {
    const int previousType = selectedNavigationType;
    const int previousId = selectedNavigationId;
    navigation->clear();

    const auto countFor = [this](const int type, const int id) {
        return static_cast<int>(std::count_if(tasks.begin(), tasks.end(),
            [type, id](const Task &task) {
                if (!isActive(task)) {
                    return false;
                }
                if (type == NAVIGATION_GROUP) {
                    return task.groupId == id;
                }
                if (type == NAVIGATION_PURSUIT) {
                    return task.pursuitId == id;
                }
                return true;
            }));
    };
    const auto addEntry = [this, &countFor, previousType, previousId](
                              const QString &name, const int type, const int id) {
        auto *item = new QListWidgetItem(
            name + " (" + QString::number(countFor(type, id)) + ")",
            navigation
        );
        item->setData(NAVIGATION_TYPE_ROLE, type);
        item->setData(NAVIGATION_ID_ROLE, id);
        if (type == previousType && id == previousId) {
            navigation->setCurrentItem(item);
        }
    };
    const auto addHeading = [this](const QString &text) {
        auto *item = new QListWidgetItem(text, navigation);
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
    addHeading("Bestrebungen");
    for (const TaskPursuit &pursuit : pursuits) {
        addEntry(pursuit.name, NAVIGATION_PURSUIT, pursuit.id);
    }

    if (!navigation->currentItem()) {
        navigation->setCurrentRow(0);
    }
}

void TaskView::renderTasks() {
    taskList->clear();

    const auto matchesSelection = [this](const Task &task) {
        if (selectedNavigationType == NAVIGATION_GROUP) {
            return task.groupId == selectedNavigationId;
        }
        if (selectedNavigationType == NAVIGATION_PURSUIT) {
            return task.pursuitId == selectedNavigationId;
        }
        return true;
    };
    const auto addTask = [this](const Task &task, const bool closed) {
        auto *item = new QTreeWidgetItem(taskList);
        item->setData(0, TASK_ID_ROLE, task.id);
        item->setText(0, task.name);
        item->setText(1, taskStateName(apiClient.metadata(), task.stateId));
        item->setText(2, taskPriorityName(apiClient.metadata(), task.priorityId));
        if (closed) {
            for (int column = 0; column < taskList->columnCount(); ++column) {
                item->setForeground(column, QColor("#777777"));
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
                separator->setForeground(0, QColor("#777777"));
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
        empty->setForeground(0, QColor("#777777"));
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
    layout->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return;
    }

    apiClient.createTask(CreateTask{
        .name = nameEdit->text().trimmed(),
        .groupId = groupEdit->currentData().toInt(),
        .priorityId = priorityEdit->currentData().toInt(),
        .pursuitId = std::nullopt
    });
}

void TaskView::openEditDialog(const Task &task) {
    QDialog dialog(this);
    dialog.setWindowTitle("Aufgabe bearbeiten");
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(task.name, &dialog);
    layout->addRow("Name", nameEdit);

    auto *stateEdit = new QComboBox(&dialog);
    for (const MetadataItem &state : apiClient.metadata().systemTaskStates) {
        stateEdit->addItem(state.displayName.value_or(state.name), state.id);
    }
    stateEdit->setCurrentIndex(stateEdit->findData(task.stateId));
    layout->addRow("Status", stateEdit);

    auto *priorityEdit = new QComboBox(&dialog);
    for (const MetadataItem &priority : apiClient.metadata().systemTaskPriorities) {
        priorityEdit->addItem(priority.displayName.value_or(priority.name), priority.id);
    }
    priorityEdit->setCurrentIndex(priorityEdit->findData(task.priorityId));
    layout->addRow("Priorität", priorityEdit);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog
    );
    auto *deleteButton = buttons->addButton("Löschen", QDialogButtonBox::DestructiveRole);
    bool deleteRequested = false;
    connect(deleteButton, &QPushButton::clicked, &dialog, [&dialog, &deleteRequested] {
        deleteRequested = true;
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    if (deleteRequested) {
        apiClient.deleteTask(task.id);
        return;
    }
    if (nameEdit->text().trimmed().isEmpty()) {
        return;
    }

    apiClient.updateTask(task.id, UpdateTask{
        .name = nameEdit->text().trimmed(),
        .stateId = stateEdit->currentData().toInt(),
        .priorityId = priorityEdit->currentData().toInt()
    });
}

void TaskView::openScheduleDialog(const Task &task) {
    const QDateTime initialStart = QDateTime::currentDateTime();
    const auto entry = showCalendarEntryDialog(this, "Aufgabe einplanen", {
        .name = task.name,
        .startAt = initialStart,
        .endAt = initialStart.addSecs(3600),
        .isReminder = true,
        .systemTaskId = task.id
    });
    if (entry) {
        apiClient.createCalendarEntry(*entry);
    }
}
