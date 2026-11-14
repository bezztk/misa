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
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *createButton = new QPushButton("Neue Aufgabe", this);
    connect(createButton, &QPushButton::clicked, this, [this] {
        openCreateDialog();
    });
    layout->addWidget(createButton, 0, Qt::AlignRight);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *cards = new QWidget(scrollArea);
    cardsLayout = new QVBoxLayout(cards);
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setSpacing(2);
    scrollArea->setWidget(cards);
    layout->addWidget(scrollArea);

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

    for (std::size_t i = 0; i < tasks.size(); ++i) {
        const Task &task = tasks[i];
        auto *card = new ClickableFrame();
        card->setCursor(Qt::PointingHandCursor);
        card->setStyleSheet(
            "QFrame { border: none; background: transparent; }"
            "QFrame:hover { background: rgba(255, 255, 255, 0.03); }"
            "QLabel { border: none; background: transparent; }"
        );

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(8, 5, 8, 5);
        cardLayout->setSpacing(1);

        auto *topRow = new QHBoxLayout();
        topRow->setContentsMargins(0, 0, 0, 0);
        topRow->setSpacing(6);
        auto *title = new QLabel(QString(
            "<span style='color: #777777; font-size: 11px; font-weight: 400'>%1::</span>"
            "<span style='color: %2'>%3</span>"
        ).arg(
            taskGroupName(task.group_id).toHtmlEscaped(),
            priorityColor(task.priority_id),
            task.name.toHtmlEscaped()
        ), card);
        title->setTextFormat(Qt::RichText);
        title->setAttribute(Qt::WA_TransparentForMouseEvents);

        auto *status = new QLabel(taskStateName(task.state_id), card);
        status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        status->setStyleSheet("color: " + taskStateColor(task.state_id) + ";");
        status->setAttribute(Qt::WA_TransparentForMouseEvents);

        topRow->addWidget(title, 1);
        topRow->addWidget(status);
        cardLayout->addLayout(topRow);

        card->clicked = [this, task] {
            openEditDialog(task);
        };

        cardsLayout->addWidget(card);

        if (i + 1 < tasks.size()) {
            auto *separator = new QFrame(this);
            separator->setFixedHeight(1);
            separator->setStyleSheet("background: rgba(255, 255, 255, 0.10); border: none;");
            cardsLayout->addWidget(separator);
        }
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
