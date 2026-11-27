#include "TaskView.h"

#include "CalendarEntryDialog.h"
#include "CommentUi.h"
#include "ContextMenuStyle.h"
#include "TaskEditDialog.h"
#include "VersionHistoryDialog.h"
#include "../api/ApiClient.h"

#include <QDateTime>
#include <QHash>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QSignalBlocker>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {
constexpr int ALL_TASKS = -1;
constexpr int WITHOUT_CONTEXT = 0;
constexpr int OPEN_TASK_COUNT_ROLE = Qt::UserRole + 1;
constexpr int CONTEXT_ICON_SIZE = 22;

class ContextNavigationDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(
        QPainter *painter,
        const QStyleOptionViewItem &option,
        const QModelIndex &index
    ) const override {
        const QRect row(0, option.rect.top(), painter->viewport().width(), option.rect.height());
        painter->save();
        painter->fillRect(row, index.data(Qt::BackgroundRole).value<QBrush>());
        if (option.state & QStyle::State_Selected) {
            QColor selection = option.palette.highlight().color();
            selection.setAlpha(70);
            painter->fillRect(row, selection);
        }

        QRect textRect = row.adjusted(18, 0, -18, 0);
        const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        if (!icon.isNull()) {
            const QRect iconRect(
                textRect.left(), row.top() + (row.height() - CONTEXT_ICON_SIZE) / 2,
                CONTEXT_ICON_SIZE, CONTEXT_ICON_SIZE
            );
            icon.paint(painter, iconRect);
            textRect.adjust(CONTEXT_ICON_SIZE + 12, 0, 0, 0);
        }

        const QString count = index.data(OPEN_TASK_COUNT_ROLE).toString();
        auto countFont = option.font;
        countFont.setPixelSize(11);
        const int countWidth = QFontMetrics(countFont).horizontalAdvance(count);
        const QRect countRect(textRect.right() + 1 - countWidth, row.top(), countWidth, row.height());
        QColor countColor = option.palette.text().color();
        countColor.setAlpha(175);
        painter->setFont(countFont);
        painter->setPen(countColor);
        painter->drawText(countRect, Qt::AlignRight | Qt::AlignVCenter, count);

        textRect.setRight(countRect.left() - 12);
        painter->setFont(option.font);
        painter->setPen(option.palette.text().color());
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
            option.fontMetrics.elidedText(index.data().toString(), Qt::ElideRight, textRect.width()));
        painter->restore();
    }
};

class TaskCardDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override {
        return {0, 72};
    }

    void paint(
        QPainter *painter,
        const QStyleOptionViewItem &option,
        const QModelIndex &index
    ) const override {
        painter->save();
        painter->setClipRect(option.rect);

        if (option.state & QStyle::State_Selected) {
            QColor selection = option.palette.highlight().color();
            selection.setAlpha(38);
            painter->fillRect(option.rect, selection);
        }
        if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, QColor(255, 255, 255, 18));
        }

        const QColor textColor = option.palette.text().color();
        auto titleFont = option.font;
        titleFont.setWeight(QFont::DemiBold);
        const QFontMetrics titleMetrics(titleFont);
        const QRect title = option.rect.adjusted(24, 12, -24, -34);
        painter->setFont(titleFont);
        painter->setPen(textColor);
        painter->drawText(title, Qt::AlignLeft | Qt::AlignVCenter,
            titleMetrics.elidedText(index.data().toString(), Qt::ElideRight, title.width()));

        QColor detailColor = textColor;
        detailColor.setAlpha(170);
        const QRect details = option.rect.adjusted(24, 35, -24, -12);
        painter->setFont(option.font);
        painter->setPen(detailColor);
        painter->drawText(details, Qt::AlignLeft | Qt::AlignVCenter,
            option.fontMetrics.elidedText(
                index.data(Qt::AccessibleDescriptionRole).toString(), Qt::ElideRight, details.width()
            ));

        if (index.row() + 1 < index.model()->rowCount()) {
            QColor separator = textColor;
            separator.setAlpha(34);
            const int inset = option.rect.width() / 10;
            const int bottom = option.rect.bottom();
            painter->setPen(separator);
            painter->drawLine(option.rect.left() + inset, bottom,
                              option.rect.right() - inset, bottom);
        }
        painter->restore();
    }
};

QString metadataName(const std::vector<MetadataItem> &items, const int id) {
    const auto item = std::ranges::find(items, id, &MetadataItem::id);
    return item == items.end() ? QString{} : item->displayName.value_or(item->name);
}
}

TaskView::TaskView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    navigation = new QListWidget(this);
    navigation->setFixedWidth(240);
    navigation->setItemDelegate(new ContextNavigationDelegate(navigation));
    navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigation->setStyleSheet(R"(
        QListWidget {
            background: transparent;
            border: 0;
            border-right: 1px solid palette(midlight);
            outline: 0;
            padding: 8px 0;
        }
    )");
    layout->addWidget(navigation);

    auto *content = new QWidget(this);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    layout->addWidget(content, 1);

    auto *searchBar = new QWidget(content);
    searchBar->setObjectName("taskSearchBar");
    searchBar->setStyleSheet(R"(
        QWidget#taskSearchBar { border-bottom: 1px solid palette(midlight); }
        QLineEdit { background: transparent; border: 0; border-radius: 0; padding: 0; }
    )");
    auto *searchLayout = new QHBoxLayout(searchBar);
    searchLayout->setContentsMargins(24, 8, 24, 8);
    search = new QLineEdit(searchBar);
    search->setFrame(false);
    search->setMinimumHeight(42);
    search->setPlaceholderText("Aufgaben suchen");
    search->setAccessibleName("Aufgaben suchen");
    search->setClearButtonEnabled(true);
    searchLayout->addWidget(search, 1);
    auto *filter = new QToolButton(searchBar);
    filter->setIcon(QIcon(":/assets/icons/filter.png"));
    filter->setIconSize(QSize(20, 20));
    filter->setAutoRaise(true);
    filter->setAccessibleName("Filter");
    filter->setEnabled(false);
    searchLayout->addWidget(filter);
    contentLayout->addWidget(searchBar);
    contentLayout->addSpacing(18);

    taskList = new QListWidget(content);
    taskList->setMaximumWidth(800);
    taskList->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    taskList->setFrameShape(QFrame::NoFrame);
    taskList->setStyleSheet("QListWidget { background: transparent; border: 0; }");
    taskList->setUniformItemSizes(true);
    taskList->setMouseTracking(true);
    taskList->viewport()->setAttribute(Qt::WA_Hover);
    taskList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    taskList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    taskList->setContextMenuPolicy(Qt::CustomContextMenu);
    taskList->setItemDelegate(new TaskCardDelegate(taskList));
    auto *taskListLayout = new QHBoxLayout;
    taskListLayout->setContentsMargins(0, 0, 0, 0);
    taskListLayout->setSpacing(0);
    taskListLayout->addStretch();
    taskListLayout->addWidget(taskList, 1);
    taskListLayout->addStretch();
    contentLayout->addLayout(taskListLayout, 1);

    connect(navigation, &QListWidget::currentRowChanged, this, &TaskView::renderTasks);
    connect(search, &QLineEdit::textChanged, this, &TaskView::renderTasks);
    connect(taskList, &QListWidget::itemActivated, this, [this](const QListWidgetItem *item) {
        showTaskEditDialog(this, apiClient, taskById(item->data(Qt::UserRole).toInt()), contexts);
    });
    connect(taskList, &QListWidget::customContextMenuRequested, this, &TaskView::showTaskMenu);
    connect(&apiClient, &ApiClient::taskReceived, this,
            [this](const std::vector<Task> &received) {
                tasks = received;
                updateNavigationCounts();
                renderTasks();
            });
    connect(&apiClient, &ApiClient::contextsReceived, this,
            [this](const std::vector<Context> &received) {
                contexts = received;
                renderNavigation();
                renderTasks();
            });
    connect(&apiClient, &ApiClient::metadataReceived, this, &TaskView::renderTasks);

    renderNavigation();
    apiClient.getContexts();
}

int TaskView::selectedContextId() const {
    const auto *item = navigation->currentItem();
    return item ? item->data(Qt::UserRole).toInt() : ALL_TASKS;
}

void TaskView::renderNavigation() {
    const QSignalBlocker blocker(navigation);
    const int selectedId = selectedContextId();
    navigation->clear();

    const auto addEntry = [&](const QString &title, const int id) {
        auto *item = new QListWidgetItem(title, navigation);
        item->setSizeHint(QSize(0, 56));
        item->setData(Qt::UserRole, id);
        if (id == selectedId) {
            navigation->setCurrentItem(item);
        }
        return item;
    };
    addEntry("Alle Aufgaben", ALL_TASKS);
    addEntry("Ohne Kontext", WITHOUT_CONTEXT);
    for (const Context &context : contexts) {
        auto *item = addEntry(context.title, context.id);
        item->setIcon(contextIcon(context.icon));
        item->setToolTip(context.title);
        if (context.color.isValid()) {
            item->setBackground(contextTint(context.color));
        }
    }
    if (!navigation->currentItem()) {
        navigation->setCurrentRow(0);
    }
    updateNavigationCounts();
}

void TaskView::updateNavigationCounts() {
    QHash<int, int> openCounts;
    for (const Task &task : tasks) {
        if (!task.isOpen()) {
            continue;
        }
        ++openCounts[ALL_TASKS];
        if (task.contextIds.empty()) {
            ++openCounts[WITHOUT_CONTEXT];
        }
        for (const int id : task.contextIds) {
            ++openCounts[id];
        }
    }
    for (int row = 0; row < navigation->count(); ++row) {
        auto *item = navigation->item(row);
        const int count = openCounts.value(item->data(Qt::UserRole).toInt());
        item->setData(OPEN_TASK_COUNT_ROLE, count);
        item->setData(Qt::AccessibleDescriptionRole, count == 1
            ? QString("1 offene Aufgabe") : QString("%1 offene Aufgaben").arg(count));
    }
}

void TaskView::renderTasks() {
    const QString query = search->text().trimmed();
    const int contextId = selectedContextId();
    const auto &metadata = apiClient.metadata();
    taskList->clear();
    for (const Task &task : tasks) {
        const bool matchesContext = contextId == ALL_TASKS
            || (contextId == WITHOUT_CONTEXT && task.contextIds.empty())
            || std::ranges::contains(task.contextIds, contextId);
        if (!matchesContext || !task.name.contains(query, Qt::CaseInsensitive)) {
            continue;
        }

        QStringList details;
        const QString state = metadataName(metadata.systemTaskStates, task.stateId);
        const QString priority = metadataName(metadata.systemTaskPriorities, task.priorityId);
        if (!state.isEmpty()) {
            details.append(state);
        }
        if (!priority.isEmpty()) {
            details.append("Priorität: " + priority);
        }
        for (const Context &context : contexts) {
            if (std::ranges::contains(task.contextIds, context.id)) {
                details.append(context.title);
            }
        }
        if (!task.comments.empty()) {
            details.append(task.comments.size() == 1 ? "1 Kommentar"
                : QString("%1 Kommentare").arg(task.comments.size()));
        }

        auto *item = new QListWidgetItem(task.name, taskList);
        item->setData(Qt::UserRole, task.id);
        item->setData(Qt::AccessibleDescriptionRole, details.join("  ·  "));
        item->setToolTip(task.name);
    }
}

const Task &TaskView::taskById(const int id) const {
    return *std::ranges::find(tasks, id, &Task::id);
}

void TaskView::openCreateDialog() {
    showTaskCreateDialog(this, apiClient, contexts, {selectedContextId()});
}

void TaskView::openScheduleDialog(const Task &task) {
    const QDateTime initialStart = QDateTime::currentDateTime();
    const auto entry = showCalendarEntryDialog(this, "Aufgabe einplanen", {
        .name = task.name,
        .startAt = initialStart,
        .endAt = initialStart.addSecs(3600),
        .isReminder = true,
        .systemTaskId = task.id
    }, "Erstellen", false);
    if (entry) {
        apiClient.createCalendarEntry(*entry);
    }
}

void TaskView::showTaskMenu(const QPoint &position) {
    const auto *item = taskList->itemAt(position);
    if (!item) {
        return;
    }
    Task task = taskById(item->data(Qt::UserRole).toInt());

    QMenu menu(this);
    const auto *edit = menu.addAction(QIcon(":/assets/icons/edit.png"), "Bearbeiten");
    const auto *schedule = menu.addAction(QIcon(":/assets/icons/clock.png"), "Einplanen");
    const auto *comments = menu.addAction(
        QIcon(":/assets/icons/speech-bubble.png"),
        task.comments.empty() ? "Kommentare anzeigen"
            : QString("Kommentare anzeigen (%1)").arg(task.comments.size())
    );
    menu.addSeparator();
    const auto *history = menu.addAction(
        QIcon(":/assets/icons/version_history.png"), "Versionshistorie"
    );
    menu.addSeparator();
    const auto *remove = menu.addAction(contextMenuDeleteIcon(), "Löschen");

    const auto *selected = menu.exec(taskList->viewport()->mapToGlobal(position));
    if (selected == edit) {
        showTaskEditDialog(this, apiClient, task, contexts);
    } else if (selected == schedule) {
        openScheduleDialog(task);
    } else if (selected == comments) {
        showCommentsDialog(this, apiClient, task.id, "Kommentare – " + task.name, task.comments);
        apiClient.getTasks();
    } else if (selected == history) {
        showVersionHistoryDialog(this, apiClient, task.id, "Versionshistorie – " + task.name);
    } else if (selected == remove) {
        apiClient.deleteTask(task.id);
    }
}
