#include "TaskView.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

static QString taskGroupName(const int id) {
    for (const TaskGroupMetadata &group : AppState::metadata.task_groups) {
        if (group.id == id) {
            return group.name;
        }
    }

    return QString::number(id);
}

static QString taskStateName(const int id) {
    for (const MetadataItem &state : AppState::metadata.task_states) {
        if (state.id == id) {
            return state.display_name.value_or(state.name);
        }
    }

    return QString::number(id);
}

static QString priorityColor(const int id) {
    switch (id) {
        case 4: return "#BA68C8";
        case 3: return "#EF5350";
        case 2: return "#64B5F6";
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
    layout->setSpacing(0);

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

    apiClient.getTasks();
}

void TaskView::renderTasks() {
    while (QLayoutItem *item = cardsLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    for (std::size_t i = 0; i < tasks.size(); ++i) {
        const Task &task = tasks[i];
        auto *card = new QFrame(this);
        card->setStyleSheet(
            "QFrame { border: none; background: transparent; }"
            "QLabel { border: none; background: transparent; }"
        );

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(8, 5, 8, 5);
        cardLayout->setSpacing(1);

        auto *topRow = new QHBoxLayout();
        topRow->setContentsMargins(0, 0, 0, 0);
        topRow->setSpacing(6);
        const QString deadline = task.due_at
            ? task.due_at->toLocalTime().toString("dd.MM.yyyy HH:mm")
            : "Keine Deadline";
        auto *title = new QLabel(QString(
            "<span style='color: #EF5350'>[%1]</span> "
            "<span style='color: #777777; font-size: 11px; font-weight: 400'>%2::</span>"
            "<span style='color: %3'>%4</span>"
        ).arg(
            deadline.toHtmlEscaped(),
            taskGroupName(task.group_id).toHtmlEscaped(),
            priorityColor(task.priority_id),
            task.name.toHtmlEscaped()
        ), card);
        title->setTextFormat(Qt::RichText);

        auto *status = new QLabel(taskStateName(task.state_id), card);
        status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        status->setStyleSheet("color: " + taskStateColor(task.state_id) + ";");

        topRow->addWidget(title, 1);
        topRow->addWidget(status);
        cardLayout->addLayout(topRow);

        if (task.description && !task.description->isEmpty()) {
            auto *description = new QLabel(*task.description, card);
            description->setWordWrap(true);
            description->setStyleSheet("color: #909090; font-size: 11px;");
            cardLayout->addWidget(description);
        }

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
