#include "TaskView.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QScrollArea>
#include <QTextEdit>
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
    connect(&apiClient, &ApiClient::taskUpdated, this, [this] {
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
        const QString deadline = task.due_at
            ? QString("<span style='color: #EF5350'>[%1]</span> ").arg(
                task.due_at->toLocalTime().toString("dd.MM.yyyy HH:mm").toHtmlEscaped()
            )
            : "";
        auto *title = new QLabel(QString(
            "%1<span style='color: #777777; font-size: 11px; font-weight: 400'>%2::</span>"
            "<span style='color: %3'>%4</span>"
        ).arg(
            deadline,
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

        if (task.description && !task.description->isEmpty()) {
            auto *description = new QLabel(*task.description, card);
            description->setWordWrap(true);
            description->setStyleSheet("color: #909090; font-size: 11px;");
            description->setAttribute(Qt::WA_TransparentForMouseEvents);
            cardLayout->addWidget(description);
        }

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

void TaskView::openEditDialog(const Task &task) {
    QDialog dialog(this);
    dialog.setWindowTitle("Task bearbeiten");

    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(task.name, &dialog);
    layout->addRow("Name", nameEdit);

    auto *stateEdit = new QComboBox(&dialog);
    for (const MetadataItem &state : AppState::metadata.task_states) {
        stateEdit->addItem(state.display_name.value_or(state.name), state.id);
    }
    stateEdit->setCurrentIndex(stateEdit->findData(task.state_id));
    layout->addRow("Status", stateEdit);

    auto *groupEdit = new QComboBox(&dialog);
    for (const TaskGroupMetadata &group : AppState::metadata.task_groups) {
        groupEdit->addItem(group.name, group.id);
    }
    groupEdit->setCurrentIndex(groupEdit->findData(task.group_id));
    layout->addRow("Gruppe", groupEdit);

    auto *priorityEdit = new QComboBox(&dialog);
    for (const MetadataItem &priority : AppState::metadata.task_priorities) {
        priorityEdit->addItem(priority.display_name.value_or(priority.name), priority.id);
    }
    priorityEdit->setCurrentIndex(priorityEdit->findData(task.priority_id));
    layout->addRow("Priorität", priorityEdit);

    auto *dueWidget = new QWidget(&dialog);
    auto *dueLayout = new QHBoxLayout(dueWidget);
    dueLayout->setContentsMargins(0, 0, 0, 0);
    auto *dueEnabled = new QCheckBox("Aktiv", dueWidget);
    auto *dueEdit = new QDateTimeEdit(dueWidget);
    dueEdit->setCalendarPopup(true);
    dueEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    dueEnabled->setChecked(task.due_at.has_value());
    dueEdit->setEnabled(task.due_at.has_value());
    dueEdit->setDateTime(task.due_at
        ? task.due_at->toLocalTime()
        : QDateTime::currentDateTime());
    connect(dueEnabled, &QCheckBox::toggled, dueEdit, &QDateTimeEdit::setEnabled);
    dueLayout->addWidget(dueEnabled);
    dueLayout->addWidget(dueEdit, 1);
    layout->addRow("Due at", dueWidget);

    auto *descriptionEdit = new QTextEdit(&dialog);
    descriptionEdit->setPlainText(task.description.value_or(""));
    descriptionEdit->setFixedHeight(90);
    layout->addRow("Beschreibung", descriptionEdit);

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

    const QString description = descriptionEdit->toPlainText().trimmed();
    apiClient.updateTask(task.entity_id, UpdateTask{
        .group_id = groupEdit->currentData().toInt(),
        .name = nameEdit->text().trimmed(),
        .description = description.isEmpty()
            ? std::nullopt
            : std::optional<QString>(description),
        .state_id = stateEdit->currentData().toInt(),
        .priority_id = priorityEdit->currentData().toInt(),
        .due_at = dueEnabled->isChecked()
            ? std::optional<QDateTime>(dueEdit->dateTime())
            : std::nullopt
    });
}
