#include "TaskEditDialog.h"
#include "DialogUtils.h"

#include "../api/ApiClient.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

#include <algorithm>

static QListWidget *addContextSelection(
    QFormLayout *layout,
    const std::vector<Context> &contexts,
    const std::vector<int> &selectedIds
) {
    auto *list = new QListWidget;
    list->setMaximumHeight(150);
    list->setSelectionMode(QAbstractItemView::NoSelection);
    for (const Context &context : contexts) {
        auto *item = new QListWidgetItem(context.title, list);
        item->setData(Qt::UserRole, context.id);
        const bool checked = std::ranges::contains(selectedIds, context.id);
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }
    layout->addRow("Kontexte", list);
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

void showTaskCreateDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const std::vector<Context> &contexts,
    const int selectedContextId
) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Aufgabe erstellen");
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(&dialog);
    layout->addRow("Name", nameEdit);

    const auto *contextSelection = addContextSelection(layout, contexts, {selectedContextId});

    auto *priorityEdit = new QComboBox(&dialog);
    for (const MetadataItem &priority : apiClient.metadata().systemTaskPriorities) {
        priorityEdit->addItem(priority.displayName.value_or(priority.name), priority.id);
    }
    layout->addRow("Priorität", priorityEdit);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog
    );
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    buttons->button(QDialogButtonBox::Save)->setText("Erstellen");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    layout->addRow(buttons);
    prepareDialog(dialog);

    if (execDialog(dialog) != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return;
    }

    apiClient.createTask(CreateTask{
        .name = nameEdit->text().trimmed(),
        .contextIds = selectedContextIds(*contextSelection),
        .priorityId = priorityEdit->currentData().toInt(),
        .domainNodeId = std::nullopt
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

    const auto *contextSelection = task.domainNodeId || contexts.empty()
        ? nullptr : addContextSelection(layout, contexts, task.contextIds);

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

    if (contextSelection) {
        const auto ids = selectedContextIds(*contextSelection);
        if (ids != task.contextIds) {
            apiClient.updateTaskContexts(task.id, ids);
        }
    }
}
