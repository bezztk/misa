#include "TaskView.h"
#include "../models/AppState.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <functional>

class ClickableFrame : public QFrame {
public:
    std::function<void()> clicked;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint())) {
            clicked();
        }

        QFrame::mouseReleaseEvent(event);
    }
};

static QString taskGroupName(const int id) {
    for (const MetadataItem &group : AppState::metadata.systemtask_groups) {
        if (group.id == id) {
            return group.name;
        }
    }

    return QString::number(id);
}

static QString taskStateName(const int id) {
    for (const MetadataItem &state : AppState::metadata.systemtask_states) {
        if (state.id == id) {
            return state.display_name.value_or(state.name);
        }
    }

    return QString::number(id);
}

static QString priorityColor(const int id) {
    switch (id) {
        case 4: return "#F02D63";
        case 3: return "#FF633F";
        case 2: return "#66BB6A";
        default: return "#78909C";
    }
}

static QString taskStateColor(const int id) {
    switch (id) {
        case 2: return "#64B5F6";
        case 3: return "#66BB6A";
        case 4: return "#EF5350";
        default: return "#777777";
    }
}

TaskView::TaskView(QWidget *parent) : QWidget(parent) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *content = new QWidget(this);
    content->setMaximumWidth(820);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(24, 20, 24, 20);
    contentLayout->setSpacing(14);

    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel("Aufgaben", content);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    title->setFont(titleFont);

    auto *createButton = new QPushButton("Neue Aufgabe", content);
    createButton->setMinimumWidth(120);
    connect(createButton, &QPushButton::clicked, this, [this] {
        openCreateDialog();
    });
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(createButton);
    contentLayout->addLayout(headerLayout);

    auto *scrollArea = new QScrollArea(content);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; }");

    auto *cards = new QWidget(scrollArea);
    cardsLayout = new QVBoxLayout(cards);
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setSpacing(8);
    scrollArea->setWidget(cards);
    contentLayout->addWidget(scrollArea, 1);

    layout->addStretch();
    layout->addWidget(content, 1);
    layout->addStretch();

    connect(&apiClient, &ApiClient::taskReceived, this,
            [this](const std::vector<Task> &receivedTasks) {
                tasks = receivedTasks;
                renderTasks();
            });
    connect(&apiClient, &ApiClient::taskUpdated, this, [this] {
        apiClient.getTasks();
    });
    connect(&apiClient, &ApiClient::taskCreated, this, [this] {
        apiClient.getTasks();
    });
    connect(&apiClient, &ApiClient::taskDeleted, this, [this] {
        apiClient.getTasks();
    });

    apiClient.getTasks();
}

void TaskView::renderTasks() {
    while (QLayoutItem *item = cardsLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    if (tasks.empty()) {
        auto *emptyLabel = new QLabel("Noch keine Aufgaben angelegt.");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #777777; padding: 28px;");
        cardsLayout->addWidget(emptyLabel);
    }

    for (const Task &task : tasks) {
        auto *card = new ClickableFrame();
        card->setObjectName("taskCard");
        card->setCursor(Qt::PointingHandCursor);
        card->setStyleSheet(QString(
            "QFrame#taskCard {"
            "  background: rgba(255, 255, 255, 0.04);"
            "  border: 1px solid rgba(255, 255, 255, 0.10);"
            "  border-left: 3px solid %1;"
            "  border-radius: 6px;"
            "}"
            "QFrame#taskCard:hover { background: rgba(255, 255, 255, 0.07); }"
            "QFrame#taskCard QLabel { border: none; background: transparent; }"
        ).arg(priorityColor(task.priority_id)));

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(12, 9, 12, 9);
        cardLayout->setSpacing(4);

        auto *topRow = new QHBoxLayout();
        topRow->setContentsMargins(0, 0, 0, 0);
        topRow->setSpacing(6);
        auto *taskTitle = new QLabel(task.name, card);
        taskTitle->setWordWrap(true);
        QFont taskTitleFont = taskTitle->font();
        taskTitleFont.setBold(true);
        taskTitle->setFont(taskTitleFont);
        taskTitle->setAttribute(Qt::WA_TransparentForMouseEvents);

        auto *status = new QLabel(taskStateName(task.state_id), card);
        status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        status->setStyleSheet(
            "color: " + taskStateColor(task.state_id)
            + "; padding: 2px 6px; background: rgba(255,255,255,0.05);"
            + " border-radius: 4px;"
        );
        status->setAttribute(Qt::WA_TransparentForMouseEvents);

        topRow->addWidget(taskTitle, 1);
        topRow->addWidget(status);
        cardLayout->addLayout(topRow);

        auto *group = new QLabel(taskGroupName(task.group_id), card);
        group->setStyleSheet("color: #777777; font-size: 11px;");
        group->setAttribute(Qt::WA_TransparentForMouseEvents);
        cardLayout->addWidget(group);

        card->clicked = [this, task] {
            openEditDialog(task);
        };

        cardsLayout->addWidget(card);
    }

    cardsLayout->addStretch();
}

void TaskView::openCreateDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Task erstellen");

    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(&dialog);
    layout->addRow("Name", nameEdit);

    auto *groupEdit = new QComboBox(&dialog);
    for (const MetadataItem &group : AppState::metadata.systemtask_groups) {
        groupEdit->addItem(group.name, group.id);
    }
    layout->addRow("Gruppe", groupEdit);

    auto *priorityEdit = new QComboBox(&dialog);
    for (const MetadataItem &priority : AppState::metadata.systemtask_priorities) {
        priorityEdit->addItem(priority.display_name.value_or(priority.name), priority.id);
    }
    layout->addRow("Priorität", priorityEdit);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return;
    }

    apiClient.createTask(CreateTask{
        .name = nameEdit->text().trimmed(),
        .group_id = groupEdit->currentData().toInt(),
        .priority_id = priorityEdit->currentData().toInt()
    });
}

void TaskView::openEditDialog(const Task &task) {
    QDialog dialog(this);
    dialog.setWindowTitle("Task bearbeiten");

    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(task.name, &dialog);
    layout->addRow("Name", nameEdit);

    auto *stateEdit = new QComboBox(&dialog);
    for (const MetadataItem &state : AppState::metadata.systemtask_states) {
        stateEdit->addItem(state.display_name.value_or(state.name), state.id);
    }
    stateEdit->setCurrentIndex(stateEdit->findData(task.state_id));
    layout->addRow("Status", stateEdit);

    auto *priorityEdit = new QComboBox(&dialog);
    for (const MetadataItem &priority : AppState::metadata.systemtask_priorities) {
        priorityEdit->addItem(priority.display_name.value_or(priority.name), priority.id);
    }
    priorityEdit->setCurrentIndex(priorityEdit->findData(task.priority_id));
    layout->addRow("Priorität", priorityEdit);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
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
        .state_id = stateEdit->currentData().toInt(),
        .priority_id = priorityEdit->currentData().toInt()
    });
}
