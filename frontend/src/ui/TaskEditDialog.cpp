#include "TaskEditDialog.h"
#include "DateTimePicker.h"
#include "DialogUtils.h"

#include "../api/ApiClient.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

#include <algorithm>

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
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(&dialog);
    layout->addRow("&Name", nameEdit);

    auto *contextSelection = domainNodeId ? nullptr : createContextSelection(&dialog, contexts, selectedIds);
    if (contextSelection) {
        layout->addRow("&Kontexte", contextSelection);
    }

    auto *priorityEdit = new QComboBox(&dialog);
    for (const MetadataItem &priority : apiClient.metadata().systemTaskPriorities) {
        priorityEdit->addItem(priority.displayName.value_or(priority.name), priority.id);
    }
    layout->addRow("&Priorität", priorityEdit);
    auto *dueDateRow = new QWidget(&dialog);
    const auto *dueDate = createDueDate(dueDateRow);
    layout->addRow("&Fälligkeit", dueDateRow);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("Erstellen");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    prepareFormDialog(dialog, *buttons);
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
    prepareFormDialog(dialog, *buttons);
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
