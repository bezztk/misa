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
#include <QPushButton>

static constexpr int WITHOUT_CONTEXT = 0;

static QComboBox *createContextSelection(
    QWidget *parent,
    const std::vector<Context> &contexts,
    const int selectedId
) {
    auto *selection = new QComboBox(parent);
    selection->addItem("Ohne Kontext", WITHOUT_CONTEXT);
    for (const Context &context : contexts) {
        selection->addItem(context.title, context.id);
    }
    selection->setCurrentIndex(qMax(0, selection->findData(selectedId)));
    return selection;
}

static std::vector<int> selectedContextIds(const QComboBox &selection) {
    const int id = selection.currentData().toInt();
    return id == WITHOUT_CONTEXT ? std::vector<int>{} : std::vector<int>{id};
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
    const int selectedContextId,
    const std::optional<int> domainNodeId
) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Aufgabe erstellen");
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(&dialog);
    layout->addRow("&Name", nameEdit);

    auto *contextSelection = domainNodeId ? nullptr : createContextSelection(&dialog, contexts, selectedContextId);
    if (contextSelection) {
        layout->addRow("&Kontext", contextSelection);
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

    const int contextId = task.contextIds.empty() ? WITHOUT_CONTEXT : task.contextIds.front();
    auto *contextSelection = task.domainNodeId || contexts.empty()
        ? nullptr : createContextSelection(&dialog, contexts, contextId);
    if (contextSelection) {
        layout->addRow("Kontext", contextSelection);
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
