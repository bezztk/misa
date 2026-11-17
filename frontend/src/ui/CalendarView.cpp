#include "CalendarView.h"
#include "CalendarEntryDialog.h"

#include <QComboBox>
#include <QDateEdit>
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

CalendarView::CalendarView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *content = new QWidget(this);
    content->setMaximumWidth(500);
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
        apiClient.generateRoutineExecutions();
    });

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(refreshButton);
    headerLayout->addWidget(createButton);
    contentLayout->addLayout(headerLayout);

    auto *filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(6);
    fromDateEdit = new QDateEdit(QDate::currentDate(), content);
    fromDateEdit->setCalendarPopup(true);
    fromDateEdit->setDisplayFormat("dd.MM.yyyy");
    toDateEdit = new QDateEdit(QDate::currentDate(), content);
    toDateEdit->setCalendarPopup(true);
    toDateEdit->setDisplayFormat("dd.MM.yyyy");
    filterLayout->addWidget(new QLabel("Von", content));
    filterLayout->addWidget(fromDateEdit);
    filterLayout->addSpacing(8);
    filterLayout->addWidget(new QLabel("Bis", content));
    filterLayout->addWidget(toDateEdit);
    filterLayout->addStretch();
    contentLayout->addLayout(filterLayout);

    tree = new QTreeWidget(content);
    tree->setColumnCount(3);
    tree->setHeaderLabels({"Zeit", "", "Name"});
    tree->setRootIsDecorated(false);
    tree->setAlternatingRowColors(true);
    tree->setUniformRowHeights(true);
    tree->setStyleSheet(R"(
        QTreeWidget {
            alternate-background-color: rgba(255, 255, 255, 0.035);
        }
        QTreeWidget::item:selected {
            background: rgba(255, 255, 255, 0.14);
        }
    )");
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    tree->header()->resizeSection(1, 32);
    tree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(fromDateEdit, &QDateEdit::dateChanged, this,
            [this] { renderEntries(); });
    connect(toDateEdit, &QDateEdit::dateChanged, this,
            [this] { renderEntries(); });
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
                QAction *deleteEntryAction = menu.addAction("Kalendereintrag löschen");
                QAction *editTaskAction = nullptr;
                QAction *deleteTaskAction = nullptr;
                if (entryIterator->systemTaskId.has_value()) {
                    menu.addSeparator();
                    editTaskAction = menu.addAction("Aufgabe bearbeiten");
                    deleteTaskAction = menu.addAction("Aufgabe löschen");
                }

                QAction *selectedAction = menu.exec(tree->viewport()->mapToGlobal(position));
                if (selectedAction == editEntryAction) {
                    openEditDialog(*entryIterator);
                } else if (selectedAction == deleteEntryAction) {
                    apiClient.deleteCalendarEntry(entryIterator->id);
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
                } else if (selectedAction == deleteTaskAction) {
                    apiClient.deleteTask(*entryIterator->systemTaskId);
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
    connect(&apiClient, &ApiClient::calendarEntryDeleted, this, [this] {
        apiClient.getCalendarEntries();
    });
    connect(&apiClient, &ApiClient::taskDeleted, this, [this] {
        apiClient.getCalendarEntries();
    });
    connect(&apiClient, &ApiClient::routineExecutionsGenerated, this, [this] {
        apiClient.getCalendarEntries();
    });
    connect(&apiClient, &ApiClient::taskReceived, this,
            [this](const std::vector<Task> &receivedTasks) {
                tasks = receivedTasks;
            });
    apiClient.getCalendarEntries();
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
        if (localStart.date() < fromDateEdit->date()
            || localStart.date() > toDateEdit->date()) {
            continue;
        }
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
        } else {
            item->setText(
                0,
                localStart.toString("HH:mm") + " – "
                    + entry.endAt->toLocalTime().toString("HH:mm")
            );
        }
        item->setText(2, entry.name);
        if (entry.systemTaskId.has_value()) {
            item->setIcon(1, QIcon(":/assets/icons/task-white.png"));
            item->setToolTip(1, "Mit einer Aufgabe verknüpft");
        } else if (entry.routineExecutionId.has_value()) {
            item->setText(1, "↻");
            item->setTextAlignment(1, Qt::AlignCenter);
            item->setToolTip(1, "Routine");
        }
    }

}

void CalendarView::openCreateDialog() {
    const QDateTime initialStart = QDateTime::currentDateTime();
    const auto entry = showCalendarEntryDialog(this, "Kalendereintrag erstellen", {
        .startAt = initialStart,
        .endAt = initialStart.addSecs(3600),
        .isReminder = true
    });
    if (entry) {
        apiClient.createCalendarEntry(*entry);
    }
}

void CalendarView::openEditDialog(const CalendarEntry &entry) {
    const auto updatedEntry = showCalendarEntryDialog(
        this,
        "Kalendereintrag bearbeiten",
        {
        .name = entry.name,
        .startAt = entry.startAt.toLocalTime(),
        .endAt = entry.endAt.has_value()
            ? std::optional(entry.endAt->toLocalTime())
            : std::nullopt,
        .isReminder = entry.isReminder(),
        .systemTaskId = entry.systemTaskId
        }
    );
    if (updatedEntry) {
        apiClient.updateCalendarEntry(entry.id, *updatedEntry);
    }
}

void CalendarView::openTaskEditDialog(const Task &task) {
    QDialog dialog(this);
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
        .stateId = stateEdit->currentData().toInt(),
        .priorityId = priorityEdit->currentData().toInt()
    });
}
