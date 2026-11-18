#include "TaskEditDialog.h"
#include "DialogUtils.h"

#include "../api/ApiClient.h"
#include "../models/Task.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>

void showTaskEditDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const Task &task
) {
    QDialog dialog(parent);
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

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("Fertig");
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addRow(buttons);
    prepareDialog(dialog);
    execDialog(dialog, true);

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
}
