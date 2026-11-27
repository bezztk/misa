#include "TaskView.h"

#include "CalendarEntryDialog.h"
#include "CommentUi.h"
#include "ContextMenuStyle.h"
#include "NavigationListDelegate.h"
#include "TaskEditDialog.h"
#include "TaskPriority.h"
#include "VersionHistoryDialog.h"
#include "../api/ApiClient.h"

#include <QDateTime>
#include <QHash>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QSignalBlocker>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <functional>
#include <utility>

namespace {
constexpr int ALL_TASKS = -1;
constexpr int WITHOUT_CONTEXT = 0;
constexpr int TASK_IMPORTANT_ROLE = Qt::UserRole + 1;
constexpr int TASK_STATUS_ROLE = Qt::UserRole + 2;
constexpr int TASK_STATUS_COLOR_ROLE = Qt::UserRole + 3;
constexpr int TASK_COMMENT_COUNT_ROLE = Qt::UserRole + 4;
constexpr int TASK_PLANNING_COUNT_ROLE = Qt::UserRole + 5;
constexpr int TASK_OWNER_ROLE = Qt::UserRole + 6;


constexpr int taskLineHeight = 22;
constexpr int priorityDotSize = 7;
constexpr int priorityWidth = priorityDotSize + 9;
constexpr int indicatorIconSize = 14;
constexpr int indicatorIconGap = 4;
constexpr int indicatorGap = 10;

struct TaskCardLine {
    TaskCardLine(const QStyleOptionViewItem &option, const QModelIndex &index)
        : titleFont(option.font), detailFont(option.font),
          owner(index.data(TASK_OWNER_ROLE).toString()), status(index.data(TASK_STATUS_ROLE).toString()) {
        titleFont.setPixelSize(14);
        titleFont.setWeight(QFont::DemiBold);
        detailFont.setPixelSize(12);
        const QFontMetrics titleMetrics(titleFont);
        const QFontMetrics detailMetrics(detailFont);

        const int lineTop = option.rect.top() + (owner.isEmpty()
            ? (option.rect.height() - taskLineHeight) / 2 : 12);
        bounds = QRect(option.rect.left() + 24, lineTop, qMax(0, option.rect.width() - 48), taskLineHeight);
        const int statusWidth = qMin(detailMetrics.horizontalAdvance(status), bounds.width());
        statusRect = QRect(bounds.right() + 1 - statusWidth, lineTop, statusWidth, taskLineHeight);
        const int titleRight = status.isEmpty() ? bounds.right() + 1 : statusRect.left() - 16;
        const int titleLeft = bounds.left() + priorityWidth;
        content = QRect(titleLeft, lineTop, qMax(0, titleRight - titleLeft), taskLineHeight);

        const auto indicatorWidth = [&detailMetrics, &index](const int role) {
            const QString count = index.data(role).toString();
            return count.isEmpty() ? 0 : indicatorIconSize + indicatorIconGap + detailMetrics.horizontalAdvance(count);
        };
        const int commentWidth = indicatorWidth(TASK_COMMENT_COUNT_ROLE);
        const int planningWidth = indicatorWidth(TASK_PLANNING_COUNT_ROLE);
        const int indicatorsWidth = commentWidth + planningWidth
            + (commentWidth > 0 ? indicatorGap : 0) + (planningWidth > 0 ? indicatorGap : 0);
        const int titleWidth = qMax(0, content.width() - indicatorsWidth);
        title = titleMetrics.elidedText(index.data().toString(), Qt::ElideRight, titleWidth);
        titleRect = QRect(titleLeft, lineTop, titleWidth, taskLineHeight);
        indicatorsLeft = titleLeft + titleMetrics.horizontalAdvance(title);
        commentRect = QRect(indicatorsLeft + indicatorGap, lineTop, commentWidth, taskLineHeight).intersected(content);
        const int planningLeft = indicatorsLeft + indicatorGap + (commentWidth > 0 ? indicatorGap + commentWidth : 0);
        planningRect = QRect(planningLeft, lineTop, planningWidth, taskLineHeight).intersected(content);
    }

    QFont titleFont;
    QFont detailFont;
    QString owner;
    QString status;
    QString title;
    QRect bounds;
    QRect content;
    QRect statusRect;
    QRect titleRect;
    QRect commentRect;
    QRect planningRect;
    int indicatorsLeft;
};

class TaskCardDelegate final : public QStyledItemDelegate {
public:
    TaskCardDelegate(std::function<void(int)> openComments, std::function<void(int)> openPlanning, QObject *parent)
        : QStyledItemDelegate(parent), openComments(std::move(openComments)), openPlanning(std::move(openPlanning)) {}

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &index) const override {
        return {0, index.data(TASK_OWNER_ROLE).toString().isEmpty() ? 56 : 72};
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
        QColor detailColor = textColor;
        detailColor.setAlpha(185);
        const TaskCardLine line(option, index);
        const QFontMetrics detailMetrics(line.detailFont);
        if (!line.status.isEmpty()) {
            painter->setFont(line.detailFont);
            painter->setPen(index.data(TASK_STATUS_COLOR_ROLE).value<QColor>());
            painter->drawText(line.statusRect,
                Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine,
                detailMetrics.elidedText(line.status, Qt::ElideRight, line.statusRect.width()));
        }

        if (index.data(TASK_IMPORTANT_ROLE).toBool()) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(importantTaskPriorityColor));
            painter->drawEllipse(QRect(line.bounds.left(), line.bounds.top() + (taskLineHeight - priorityDotSize) / 2,
                priorityDotSize, priorityDotSize));
            painter->restore();
        }

        painter->save();
        painter->setClipRect(line.content, Qt::IntersectClip);
        painter->setFont(line.titleFont);
        painter->setPen(textColor);
        painter->drawText(line.titleRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, line.title);

        struct Indicator {
            const QIcon &icon;
            QString text;
        };
        const std::array indicators{
            Indicator{commentIcon, index.data(TASK_COMMENT_COUNT_ROLE).toString()},
            Indicator{planningIcon, index.data(TASK_PLANNING_COUNT_ROLE).toString()}
        };
        const int y = line.bounds.top();
        int x = line.indicatorsLeft;
        painter->setFont(line.detailFont);
        painter->setPen(detailColor);
        for (const auto &indicator : indicators) {
            if (!indicator.text.isEmpty()) {
                x += indicatorGap;
                indicator.icon.paint(painter, QRect(x, y + (taskLineHeight - indicatorIconSize) / 2,
                    indicatorIconSize, indicatorIconSize));
                x += indicatorIconSize + indicatorIconGap;
                const int width = detailMetrics.horizontalAdvance(indicator.text);
                painter->drawText(QRect(x, y, width, taskLineHeight),
                    Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, indicator.text);
                x += width;
            }
        }
        painter->restore();

        if (!line.owner.isEmpty()) {
            const QRect ownerLine = line.bounds.translated(0, taskLineHeight).adjusted(priorityWidth, 0, 0, 0);
            painter->setFont(line.detailFont);
            painter->setPen(detailColor);
            painter->drawText(ownerLine, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                detailMetrics.elidedText(line.owner, Qt::ElideRight, ownerLine.width()));
        }

        if (index.row() + 1 < index.model()->rowCount()) {
            QColor separator = textColor;
            separator.setAlpha(34);
            const int bottom = option.rect.bottom();
            painter->setPen(separator);
            painter->drawLine(option.rect.left(), bottom, option.rect.right(), bottom);
        }
        painter->restore();
    }

    bool editorEvent(
        QEvent *event,
        QAbstractItemModel *model,
        const QStyleOptionViewItem &option,
        const QModelIndex &index
    ) override {
        const auto type = event->type();
        if (type != QEvent::MouseButtonPress && type != QEvent::MouseButtonRelease
            && type != QEvent::MouseButtonDblClick) {
            return QStyledItemDelegate::editorEvent(event, model, option, index);
        }
        const auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() != Qt::LeftButton) {
            return false;
        }
        const TaskCardLine line(option, index);
        const auto point = mouse->position().toPoint();
        const bool planning = line.planningRect.contains(point);
        const bool hit = planning || line.commentRect.contains(point);
        if (type == QEvent::MouseButtonRelease) {
            const bool clicked = hit && pressedItem == index && pressedPlanning == planning;
            pressedItem = QModelIndex{};
            if (clicked) {
                (planning ? openPlanning : openComments)(index.data(Qt::UserRole).toInt());
            }
        } else {
            pressedItem = hit ? index : QModelIndex{};
            pressedPlanning = planning;
        }
        return hit;
    }

private:
    std::function<void(int)> openComments;
    std::function<void(int)> openPlanning;
    QPersistentModelIndex pressedItem;
    bool pressedPlanning = false;
    const QIcon commentIcon{":/assets/icons/speech-bubble.png"};
    const QIcon planningIcon{":/assets/icons/clock.png"};
};

QColor taskStatusColor(const QString &name) {
    if (name == "in_progress") return QColor("#84B7EC");
    if (name == "completed") return QColor("#83C795");
    if (name == "cancelled") return QColor("#E68484");
    return {};
}
}

TaskView::TaskView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    navigation = new QListWidget(this);
    navigation->setFixedWidth(300);
    QFont contextFont(QStringLiteral("Segoe UI"));
    contextFont.setPixelSize(15);
    contextFont.setWeight(QFont::Medium);
    navigation->setFont(contextFont);
    navigation->setMouseTracking(true);
    navigation->viewport()->setAttribute(Qt::WA_Hover);
    navigation->setItemDelegate(new NavigationListDelegate(navigation));
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
        QLineEdit { background: transparent; border: 0; border-radius: 0; padding: 0; font-size: 14px; }
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
    taskList->setMouseTracking(true);
    taskList->viewport()->setAttribute(Qt::WA_Hover);
    taskList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    taskList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    taskList->setContextMenuPolicy(Qt::CustomContextMenu);
    taskList->setItemDelegate(new TaskCardDelegate([this](const int id) {
        showTaskComments(taskById(id));
    }, [this](const int id) {
        showTaskPlanningDialog(this, apiClient, taskById(id));
    }, taskList));
    auto *taskListLayout = new QHBoxLayout;
    taskListLayout->setContentsMargins(0, 0, 0, 0);
    taskListLayout->setSpacing(0);
    taskListLayout->addStretch();
    taskListLayout->addWidget(taskList, 1);
    taskListLayout->addStretch();
    contentLayout->addLayout(taskListLayout, 1);

    connect(navigation, &QListWidget::currentRowChanged, this, &TaskView::renderTasks);
    connect(navigation, &QListWidget::itemEntered, this, [this](const QListWidgetItem *item) {
        navigation->viewport()->setCursor(item->flags().testFlag(Qt::ItemIsEnabled)
            ? Qt::PointingHandCursor : Qt::ArrowCursor);
    });
    connect(navigation, &QListWidget::viewportEntered, this, [this] {
        navigation->viewport()->unsetCursor();
    });
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
    if (!contexts.empty()) {
        auto *divider = new QListWidgetItem(navigation);
        divider->setFlags(Qt::NoItemFlags);
        divider->setSizeHint(QSize(0, 20));
    }
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
        if (!item->flags().testFlag(Qt::ItemIsEnabled)) {
            continue;
        }
        const int count = openCounts.value(item->data(Qt::UserRole).toInt());
        item->setData(NavigationListDelegate::CountRole, count);
        item->setData(Qt::AccessibleDescriptionRole, count == 1
            ? QString("1 offene Aufgabe") : QString("%1 offene Aufgaben").arg(count));
    }
}

void TaskView::renderTasks() {
    const QString query = search->text().trimmed();
    const int contextId = selectedContextId();
    const auto &metadata = apiClient.metadata();
    const auto *currentItem = taskList->currentItem();
    const std::optional<int> selectedId = taskToShow ? taskToShow
        : currentItem ? std::optional(currentItem->data(Qt::UserRole).toInt()) : std::nullopt;
    QListWidgetItem *selectedItem = nullptr;
    taskList->clear();
    for (const Task &task : tasks) {
        const bool matchesContext = contextId == ALL_TASKS
            || (contextId == WITHOUT_CONTEXT && task.contextIds.empty())
            || std::ranges::contains(task.contextIds, contextId);
        if (!matchesContext || !task.name.contains(query, Qt::CaseInsensitive)) {
            continue;
        }

        auto *item = new QListWidgetItem(task.name);
        item->setData(Qt::UserRole, task.id);

        QStringList details;
        const auto state = std::ranges::find(metadata.systemTaskStates, task.stateId, &MetadataItem::id);
        if (state != metadata.systemTaskStates.end()) {
            const QColor color = taskStatusColor(state->name);
            if (color.isValid()) {
                const QString status = state->displayName.value_or(state->name);
                item->setData(TASK_STATUS_ROLE, status);
                item->setData(TASK_STATUS_COLOR_ROLE, color);
                details.append(status);
            }
        }
        if (const auto *priority = importantTaskPriority(metadata, task.priorityId)) {
            item->setData(TASK_IMPORTANT_ROLE, true);
            details.append("Priorität: " + priority->displayName.value_or(priority->name));
        }
        if (!task.comments.empty()) {
            item->setData(TASK_COMMENT_COUNT_ROLE, static_cast<qint64>(task.comments.size()));
            details.append(task.comments.size() == 1 ? "1 Kommentar"
                : QString("%1 Kommentare").arg(task.comments.size()));
        }
        if (task.planningCount > 0) {
            item->setData(TASK_PLANNING_COUNT_ROLE, task.planningCount);
            details.append(task.planningCount == 1 ? "1 Planung"
                : QString("%1 Planungen").arg(task.planningCount));
        }
        if (!task.domainNodeName.isEmpty()) {
            const QString owner = "Aufschrieb: " + task.domainNodeName;
            item->setData(TASK_OWNER_ROLE, owner);
            details.append(owner);
        }
        item->setData(Qt::AccessibleDescriptionRole, details.join("  ·  "));
        item->setToolTip(task.name);
        taskList->addItem(item);
        if (selectedId == task.id) {
            selectedItem = item;
        }
    }
    if (selectedItem) {
        taskList->setCurrentItem(selectedItem);
        taskList->scrollToItem(selectedItem);
        taskToShow.reset();
    }
}

const Task &TaskView::taskById(const int id) const {
    return *std::ranges::find(tasks, id, &Task::id);
}

void TaskView::openCreateDialog() {
    showTaskCreateDialog(this, apiClient, contexts, selectedContextId());
}

void TaskView::showTask(const int id) {
    const QSignalBlocker searchBlocker(search);
    const QSignalBlocker navigationBlocker(navigation);
    search->clear();
    navigation->setCurrentRow(0);
    taskToShow = id;
    renderTasks();
    taskList->setFocus();
}

void TaskView::openScheduleDialog(const Task &task, const bool isReminder) {
    if (const auto entry = showTaskScheduleDialog(this, task, isReminder)) {
        apiClient.createCalendarEntry(*entry);
    }
}

void TaskView::showTaskComments(const Task &task) {
    auto comments = task.comments;
    showCommentsDialog(this, apiClient, task.id, "Kommentare – " + task.name, comments);
    apiClient.getTasks();
}

void TaskView::showTaskMenu(const QPoint &position) {
    const auto *item = taskList->itemAt(position);
    if (!item) {
        return;
    }
    Task task = taskById(item->data(Qt::UserRole).toInt());

    QMenu menu(this);
    const auto *edit = menu.addAction(QIcon(":/assets/icons/edit.png"), "Bearbeiten");
    auto *scheduleMenu = menu.addMenu(QIcon(":/assets/icons/clock.png"), "Einplanen");
    const auto *reminder = scheduleMenu->addAction("Erinnerung");
    const auto *appointment = scheduleMenu->addAction("Termin");
    scheduleMenu->addAction("Routine")->setEnabled(false);
    const auto *planning = menu.addAction(QIcon(":/assets/icons/calendar.png"), "Zeitplanung anzeigen");
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
    } else if (selected == reminder || selected == appointment) {
        openScheduleDialog(task, selected == reminder);
    } else if (selected == planning) {
        showTaskPlanningDialog(this, apiClient, task);
    } else if (selected == comments) {
        showTaskComments(task);
    } else if (selected == history) {
        showVersionHistoryDialog(this, apiClient, task.id, "Versionshistorie – " + task.name);
    } else if (selected == remove) {
        apiClient.deleteTask(task.id);
    }
}
