#include "TaskEditDialog.h"
#include "DateTimePicker.h"
#include "DialogUtils.h"

#include "../api/ApiClient.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

static constexpr auto TASK_CREATE_STYLE = R"(
    QLabel#taskFieldLabel {
        color: #D2D2D2;
        font-family: "Bahnschrift";
        font-size: 13px;
        font-weight: 500;
    }
    QLineEdit#taskNameField {
        background: #2B2B2B;
        color: #F4F4F4;
        border: 1px solid #5E5E5E;
        border-radius: 0;
        padding: 0 12px;
        font-family: "Bahnschrift";
        font-size: 14px;
        selection-background-color: #626262;
        selection-color: #FFFFFF;
    }
    QLineEdit#taskNameField:focus {
        border-color: #B8B8B8;
    }
    QFrame#taskCreateFooter {
        border: 0;
        border-top: 1px solid #414141;
    }
    QPushButton#taskCreateCancel, QPushButton#taskClearDueDate,
    QPushButton#taskCreateSubmit {
        min-height: 36px;
        padding: 0 15px;
        font-family: "Bahnschrift";
        font-size: 14px;
        font-weight: 500;
        border: 1px solid #5E5E5E;
        border-radius: 0;
        color: #F4F4F4;
        background: transparent;
    }
    QPushButton#taskCreateCancel:hover, QPushButton#taskClearDueDate:hover {
        background: #353535;
    }
    QPushButton#taskCreateCancel:pressed, QPushButton#taskClearDueDate:pressed {
        background: #414141;
    }
    QPushButton#taskClearDueDate:disabled {
        color: #929292;
        border-color: #414141;
    }
    QPushButton#taskCreateSubmit {
        min-width: 80px;
        color: #202020;
        background: #D2D2D2;
        border-color: #D2D2D2;
    }
    QPushButton#taskCreateSubmit:hover {
        background: #E6E6E6;
    }
    QPushButton#taskCreateSubmit:pressed {
        background: #B8B8B8;
    }
    QPushButton#taskCreateCancel:focus, QPushButton#taskClearDueDate:focus,
    QPushButton#taskCreateSubmit:focus {
        border-color: #FFFFFF;
    }
)";

static void addTaskField(QVBoxLayout *layout, const QString &title, QWidget *field) {
    auto *group = new QVBoxLayout;
    group->setSpacing(6);
    auto *label = new QLabel(title, layout->parentWidget());
    label->setObjectName("taskFieldLabel");
    label->setBuddy(field);
    group->addWidget(label);
    group->addWidget(field);
    layout->addLayout(group);
}

static QListWidget *createContextSelection(
    QWidget *parent,
    const std::vector<Context> &contexts,
    const std::vector<int> &selectedIds
) {
    auto *list = new QListWidget(parent);
    list->setMaximumHeight(150);
    list->setSelectionMode(QAbstractItemView::NoSelection);
    for (const Context &context : contexts) {
        auto *item = new QListWidgetItem(context.title, list);
        item->setData(Qt::UserRole, context.id);
        const bool checked = std::ranges::contains(selectedIds, context.id);
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }
    return list;
}

static std::vector<int> selectedContextIds(const QListWidget &list) {
    std::vector<int> ids;
    for (int row = 0; row < list.count(); ++row) {
        const auto *item = list.item(row);
        if (item->checkState() == Qt::Checked) {
            ids.push_back(item->data(Qt::UserRole).toInt());
        }
    }
    std::ranges::sort(ids);
    return ids;
}

static DateTimeButton *createDueDate(QWidget *row, const QDateTime &dueAt = {}) {
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(8);

    auto *dueDate = new DateTimeButton("Fälligkeit auswählen", dueAt, row);
    auto *clear = new QPushButton("Entfernen", row);
    clear->setObjectName("taskClearDueDate");
    clear->setAccessibleName("Fälligkeit entfernen");
    clear->setAutoDefault(false);
    clear->setEnabled(dueAt.isValid());
    QObject::connect(clear, &QPushButton::clicked, dueDate, [dueDate] {
        dueDate->setDateTime({});
    });
    QObject::connect(dueDate, &DateTimeButton::dateTimeChanged, clear, [dueDate, clear] {
        clear->setEnabled(dueDate->dateTime().isValid());
    });
    rowLayout->addWidget(dueDate, 1);
    rowLayout->addWidget(clear);
    row->setFocusProxy(dueDate);
    return dueDate;
}

void showTaskCreateDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const std::vector<Context> &contexts,
    const std::vector<int> &selectedIds,
    const std::optional<int> domainNodeId
) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Aufgabe erstellen");
    dialog.setStyleSheet(TASK_CREATE_STYLE);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);

    auto *nameEdit = new QLineEdit(&dialog);
    nameEdit->setObjectName("taskNameField");
    nameEdit->setFixedHeight(38);
    addTaskField(layout, "&Name", nameEdit);

    auto *contextSelection = domainNodeId ? nullptr : createContextSelection(&dialog, contexts, selectedIds);
    if (contextSelection) {
        addTaskField(layout, "&Kontexte", contextSelection);
    }

    auto *priorityEdit = new QComboBox(&dialog);
    for (const MetadataItem &priority : apiClient.metadata().systemTaskPriorities) {
        priorityEdit->addItem(priority.displayName.value_or(priority.name), priority.id);
    }
    addTaskField(layout, "&Priorität", priorityEdit);
    auto *dueDateRow = new QWidget(&dialog);
    const auto *dueDate = createDueDate(dueDateRow);
    addTaskField(layout, "&Fälligkeit", dueDateRow);

    auto *footer = new QFrame(&dialog);
    footer->setObjectName("taskCreateFooter");
    auto *actions = new QHBoxLayout(footer);
    actions->setContentsMargins(0, 12, 0, 0);
    actions->setSpacing(8);
    actions->addStretch();
    auto *cancel = new QPushButton("Abbrechen", footer);
    cancel->setObjectName("taskCreateCancel");
    cancel->setAutoDefault(false);
    actions->addWidget(cancel);
    auto *create = new QPushButton("Erstellen", footer);
    create->setObjectName("taskCreateSubmit");
    create->setDefault(true);
    actions->addWidget(create);
    layout->addSpacing(4);
    layout->addWidget(footer);
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(create, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.setMinimumWidth(460);
    dialog.resize(460, dialog.minimumSizeHint().height());
    nameEdit->setFocus();

    if (execDialog(dialog) != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return;
    }

    apiClient.createTask(CreateTask{
        .name = nameEdit->text().trimmed(),
        .contextIds = contextSelection ? selectedContextIds(*contextSelection) : std::vector<int>{},
        .priorityId = priorityEdit->currentData().toInt(),
        .domainNodeId = domainNodeId,
        .dueAt = dueDate->dateTime()
    });
}

void showTaskEditDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const Task task,
    const std::vector<Context> &contexts
) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Aufgabe bearbeiten");
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(task.name, &dialog);
    layout->addRow("Name", nameEdit);

    auto *contextSelection = task.domainNodeId || contexts.empty()
        ? nullptr : createContextSelection(&dialog, contexts, task.contextIds);
    if (contextSelection) {
        layout->addRow("Kontexte", contextSelection);
    }

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
    auto *dueDateRow = new QWidget(&dialog);
    const auto *dueDate = createDueDate(dueDateRow, task.dueAt);
    layout->addRow("&Fälligkeit", dueDateRow);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("Fertig");
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addRow(buttons);
    prepareDialog(dialog);
    if (execDialog(dialog, true) != QDialog::Accepted) {
        return;
    }

    const QString name = nameEdit->text().trimmed();
    if (!name.isEmpty() && name != task.name) {
        apiClient.updateTaskField(task.id, TaskField::Name, name);
    }

    const int stateId = stateEdit->currentData().toInt();
    if (stateId != task.stateId) {
        apiClient.updateTaskField(task.id, TaskField::StateId, stateId);
    }

    const int priorityId = priorityEdit->currentData().toInt();
    if (priorityId != task.priorityId) {
        apiClient.updateTaskField(task.id, TaskField::PriorityId, priorityId);
    }

    const QDateTime dueAt = dueDate->dateTime();
    if (dueAt != task.dueAt) {
        const QJsonValue value = dueAt.isValid()
            ? QJsonValue(dueAt.toUTC().toString(Qt::ISODateWithMs))
            : QJsonValue(QJsonValue::Null);
        apiClient.updateTaskField(task.id, TaskField::DueAt, value);
    }

    if (contextSelection) {
        const auto ids = selectedContextIds(*contextSelection);
        if (ids != task.contextIds) {
            apiClient.updateTaskContexts(task.id, ids);
        }
    }
}
