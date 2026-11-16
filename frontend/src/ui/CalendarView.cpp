#include "CalendarView.h"
#include "../models/AppState.h"

#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

CalendarView::CalendarView(QWidget *parent) : QWidget(parent) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *content = new QWidget(this);
    content->setMaximumWidth(1000);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(24, 20, 24, 20);
    contentLayout->setSpacing(14);

    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel("Kalender", content);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    title->setFont(titleFont);

    auto *createButton = new QPushButton("Neuer Eintrag", content);
    connect(createButton, &QPushButton::clicked, this, [this] {
        openCreateDialog();
    });

    auto *refreshButton = new QPushButton("Aktualisieren", content);
    connect(refreshButton, &QPushButton::clicked, this, [this] {
        apiClient.getCalendarEntries();
    });

    auto *routinesButton = new QPushButton("Routinen berechnen", content);
    connect(routinesButton, &QPushButton::clicked, this, [this] {
        apiClient.generateRoutineExecutions();
    });

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(routinesButton);
    headerLayout->addWidget(refreshButton);
    headerLayout->addWidget(createButton);
    contentLayout->addLayout(headerLayout);

    tree = new QTreeWidget(content);
    tree->setColumnCount(3);
    tree->setHeaderLabels({"Zeit", "Name", "Art"});
    tree->setRootIsDecorated(false);
    tree->setAlternatingRowColors(true);
    tree->setUniformRowHeights(true);
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tree, &QWidget::customContextMenuRequested, this,
            [this](const QPoint &position) {
                QTreeWidgetItem *item = tree->itemAt(position);
                if (item == nullptr || !item->data(0, Qt::UserRole).isValid()) {
                    return;
                }

                const int entryId = item->data(0, Qt::UserRole).toInt();
                const auto entryIterator = std::find_if(
                    entries.begin(), entries.end(), [entryId](const CalendarEntry &entry) {
                        return entry.id == entryId;
                    }
                );
                if (entryIterator == entries.end()) {
                    return;
                }

                QMenu menu(tree);
                QAction *editEntryAction = menu.addAction("Kalendereintrag bearbeiten");
                QAction *editTaskAction = nullptr;
                if (entryIterator->systemTaskId.has_value()) {
                    editTaskAction = menu.addAction("Aufgabe bearbeiten");
                }

                QAction *selectedAction = menu.exec(tree->viewport()->mapToGlobal(position));
                if (selectedAction == editEntryAction) {
                    openEditDialog(*entryIterator);
                } else if (selectedAction == editTaskAction) {
                    const int taskId = *entryIterator->systemTaskId;
                    const auto taskIterator = std::find_if(
                        tasks.begin(), tasks.end(), [taskId](const Task &task) {
                            return task.id == taskId;
                        }
                    );
                    if (taskIterator != tasks.end()) {
                        openTaskEditDialog(*taskIterator);
                    }
                }
            });
    contentLayout->addWidget(tree, 1);

    layout->addStretch();
    layout->addWidget(content, 1);
    layout->addStretch();

    connect(&apiClient, &ApiClient::calendarEntriesReceived, this,
            [this](const std::vector<CalendarEntry> &receivedEntries) {
                entries = receivedEntries;
                renderEntries();
            });
    connect(&apiClient, &ApiClient::calendarEntryCreated, this, [this] {
        apiClient.getCalendarEntries();
    });
    connect(&apiClient, &ApiClient::calendarEntryUpdated, this, [this] {
        apiClient.getCalendarEntries();
    });
    connect(&apiClient, &ApiClient::routineExecutionsGenerated, this, [this] {
        apiClient.getCalendarEntries();
    });
    connect(&apiClient, &ApiClient::taskReceived, this,
            [this](const std::vector<Task> &receivedTasks) {
                tasks = receivedTasks;
            });
    connect(&apiClient, &ApiClient::taskUpdated, this, [this] {
        apiClient.getTasks();
    });

    apiClient.getCalendarEntries();
    apiClient.getTasks();
}

void CalendarView::renderEntries() {
    tree->clear();

    std::sort(entries.begin(), entries.end(), [](const CalendarEntry &left,
                                                  const CalendarEntry &right) {
        return left.startAt > right.startAt;
    });

    QDate currentDate;
    const QLocale locale(QLocale::German, QLocale::Germany);

    for (const CalendarEntry &entry : entries) {
        const QDateTime localStart = entry.startAt.toLocalTime();
        if (localStart.date() != currentDate) {
            currentDate = localStart.date();
            auto *dateItem = new QTreeWidgetItem(tree);
            dateItem->setText(0, locale.toString(currentDate, "dddd, d. MMMM yyyy"));
            QFont dateFont = dateItem->font(0);
            dateFont.setBold(true);
            dateItem->setFont(0, dateFont);
            dateItem->setFirstColumnSpanned(true);
        }

        auto *item = new QTreeWidgetItem(tree);
        item->setData(0, Qt::UserRole, entry.id);
        if (entry.isReminder()) {
            item->setText(0, localStart.toString("HH:mm"));
            item->setText(2, "Erinnerung");
        } else {
            item->setText(
                0,
                localStart.toString("HH:mm") + " – "
                    + entry.endAt->toLocalTime().toString("HH:mm")
            );
            item->setText(2, "Termin");
        }
        item->setText(1, entry.name);
        if (entry.systemTaskId.has_value()) {
            item->setIcon(1, QIcon(":/assets/icons/task-white.png"));
            item->setToolTip(1, "Mit einer Aufgabe verknüpft");
        }

        const QColor color = entry.isReminder() ? QColor("#FFB74D") : QColor("#64B5F6");
        for (int column = 0; column < tree->columnCount(); ++column) {
            item->setForeground(column, color);
        }
    }

}

void CalendarView::openCreateDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Kalendereintrag erstellen");

    auto *layout = new QFormLayout(&dialog);

    auto *typeEdit = new QComboBox(&dialog);
    typeEdit->addItem("Erinnerung", true);
    typeEdit->addItem("Termin", false);
    layout->addRow("Art", typeEdit);

    auto *nameEdit = new QLineEdit(&dialog);
    layout->addRow("Name", nameEdit);

    const QDateTime initialStart = QDateTime::currentDateTime();
    auto *startEdit = new QDateTimeEdit(initialStart, &dialog);
    startEdit->setCalendarPopup(true);
    startEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    layout->addRow("Start", startEdit);

    auto *endLabel = new QLabel("Ende", &dialog);
    auto *endEdit = new QDateTimeEdit(initialStart.addSecs(3600), &dialog);
    endEdit->setCalendarPopup(true);
    endEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    endEdit->setMinimumDateTime(initialStart.addSecs(60));
    layout->addRow(endLabel, endEdit);

    const auto updateType = [typeEdit, endLabel, endEdit] {
        const bool isReminder = typeEdit->currentData().toBool();
        endLabel->setVisible(!isReminder);
        endEdit->setVisible(!isReminder);
    };
    connect(typeEdit, &QComboBox::currentIndexChanged, &dialog, updateType);
    connect(startEdit, &QDateTimeEdit::dateTimeChanged, &dialog,
            [endEdit](const QDateTime &start) {
                endEdit->setMinimumDateTime(start.addSecs(60));
            });
    updateType();

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

    const bool isReminder = typeEdit->currentData().toBool();
    CreateCalendarEntry entry{
        .name = nameEdit->text().trimmed(),
        .startAt = startEdit->dateTime(),
        .isReminder = isReminder
    };
    if (!isReminder) {
        entry.endAt = endEdit->dateTime();
    }

    apiClient.createCalendarEntry(entry);
}

void CalendarView::openEditDialog(const CalendarEntry &entry) {
    QDialog dialog(this);
    dialog.setWindowTitle("Kalendereintrag bearbeiten");

    auto *layout = new QFormLayout(&dialog);

    auto *typeEdit = new QComboBox(&dialog);
    typeEdit->addItem("Erinnerung", true);
    typeEdit->addItem("Termin", false);
    typeEdit->setCurrentIndex(entry.isReminder() ? 0 : 1);
    layout->addRow("Art", typeEdit);

    auto *nameEdit = new QLineEdit(entry.name, &dialog);
    layout->addRow("Name", nameEdit);

    auto *startEdit = new QDateTimeEdit(entry.startAt.toLocalTime(), &dialog);
    startEdit->setCalendarPopup(true);
    startEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    layout->addRow("Start", startEdit);

    auto *endLabel = new QLabel("Ende", &dialog);
    auto *endEdit = new QDateTimeEdit(
        entry.endAt.has_value()
            ? entry.endAt->toLocalTime()
            : entry.startAt.toLocalTime().addSecs(3600),
        &dialog
    );
    endEdit->setCalendarPopup(true);
    endEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    endEdit->setMinimumDateTime(startEdit->dateTime().addSecs(60));
    layout->addRow(endLabel, endEdit);

    const auto updateType = [typeEdit, endLabel, endEdit] {
        const bool isReminder = typeEdit->currentData().toBool();
        endLabel->setVisible(!isReminder);
        endEdit->setVisible(!isReminder);
    };
    connect(typeEdit, &QComboBox::currentIndexChanged, &dialog, updateType);
    connect(startEdit, &QDateTimeEdit::dateTimeChanged, &dialog,
            [endEdit](const QDateTime &start) {
                endEdit->setMinimumDateTime(start.addSecs(60));
            });
    updateType();

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

    const bool isReminder = typeEdit->currentData().toBool();
    CreateCalendarEntry updatedEntry{
        .name = nameEdit->text().trimmed(),
        .startAt = startEdit->dateTime(),
        .isReminder = isReminder,
        .systemTaskId = entry.systemTaskId
    };
    if (!isReminder) {
        updatedEntry.endAt = endEdit->dateTime();
    }

    apiClient.updateCalendarEntry(entry.id, updatedEntry);
}

void CalendarView::openTaskEditDialog(const Task &task) {
    QDialog dialog(this);
    dialog.setWindowTitle("Aufgabe bearbeiten");

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
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return;
    }

    apiClient.updateTask(task.id, UpdateTask{
        .name = nameEdit->text().trimmed(),
        .state_id = stateEdit->currentData().toInt(),
        .priority_id = priorityEdit->currentData().toInt()
    });
}
