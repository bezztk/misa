#include "CalendarView.h"

#include "../api/ApiClient.h"
#include "CalendarEntryDialog.h"
#include "ContextMenuStyle.h"
#include "TaskPriority.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <array>

namespace {

constexpr int hourHeight = 68;
constexpr int timeGutterWidth = 64;
constexpr int gridPadding = 12;
constexpr int entryMinimumHeight = 20;
constexpr int millisecondsPerHour = 3'600'000;
constexpr int daysPerWeek = 7;
constexpr int weekdayHeaderHeight = 28;
constexpr auto taskIconPath = ":/assets/icons/task-white.png";
constexpr auto nodeIconPath = ":/assets/icons/node.png";

const QLocale german(QLocale::German, QLocale::Germany);

constexpr auto toolButtonStyle = R"(
    QToolButton {
        background: #242424;
        color: #E5E5E5;
        border: 1px solid #414141;
        border-radius: 0;
        padding: 0 6px;
    }
    QToolButton:hover, QToolButton:pressed { background: #333333; }
    QToolButton:checked { background: #344553; border-color: #526B7D; }
    QToolButton:focus { border-color: #777777; }
)";

QDate mondayOf(const QDate date) {
    return date.addDays(1 - date.dayOfWeek());
}

bool occursOnDay(const CalendarEntry &entry, const QDateTime &start, const QDateTime &end) {
    return entry.startAt < end && (entry.endAt ? *entry.endAt > start : entry.startAt >= start);
}

class CalendarEntryButton final : public QAbstractButton {
public:
    CalendarEntryButton(const CalendarEntry &entry, const Task *task, const Context *context,
                        const bool important, QWidget *parent)
        : QAbstractButton(parent),
          title(entry.name),
          startTime(entry.startAt.toLocalTime().toString("HH:mm")),
          time(startTime),
          nodeName(task ? task->domainNodeName : QString()),
          contextName(context ? context->title : QString()),
          contextImage(context ? contextIcon(context->icon) : QIcon()),
          typeIcon(entry.routineExecutionId ? ":/assets/icons/infinity.png" : entry.systemTaskId ? taskIconPath
                   : entry.isReminder() ? ":/assets/icons/clock.png" : ":/assets/icons/calendar.png"),
          fill(entry.routineExecutionId ? "#344934" : entry.systemTaskId ? "#354C60" : "#30485B"),
          accent(context && context->color.isValid() ? context->color
                 : QColor(entry.isReminder() ? "#D39A55" : entry.routineExecutionId ? "#819D78" : "#7799B3")),
          important(important),
          taskIsTypeIcon(entry.systemTaskId.has_value()),
          taskAction(task ? createLink(taskIconPath, "Aufgabe öffnen · " + task->name) : nullptr),
          nodeAction(task && task->domainNodeId ? createLink(nodeIconPath, "Aufschrieb öffnen · " + nodeName) : nullptr) {
        const auto dateFormat = "dd.MM.yyyy, HH:mm";
        QString details = german.toString(entry.startAt.toLocalTime(), dateFormat);
        if (entry.endAt) {
            time += " – " + entry.endAt->toLocalTime().toString("HH:mm");
            details += " – " + german.toString(entry.endAt->toLocalTime(), dateFormat);
        }
        if (important) {
            details += " · Hohe Priorität";
        }
        if (!nodeName.isEmpty()) {
            details += " · Aufschrieb: " + nodeName;
        }
        if (!contextName.isEmpty()) {
            details += " · Kontext: " + contextName;
        }
        if (entry.isLocked) {
            details += " · Gesperrt";
        }

        setAccessibleName(title);
        setAccessibleDescription(details);
        setAttribute(Qt::WA_Hover);
        setFocusPolicy(Qt::StrongFocus);
        setCursor(Qt::PointingHandCursor);
        setContextMenuPolicy(Qt::CustomContextMenu);
    }

    QToolButton *taskButton() const { return taskAction; }
    QToolButton *nodeButton() const { return nodeAction; }

protected:
    void resizeEvent(QResizeEvent *) override {
        const QRect row = firstRow();
        if (taskAction && taskIsTypeIcon) {
            taskAction->setGeometry(iconRect(row.left(), row));
            taskAction->setVisible(row.width() >= iconWidth);
        }

        int right = row.right() + 1;
        for (auto *action : {taskIsTypeIcon ? nullptr : taskAction, nodeAction}) {
            if (action) {
                const bool visible = right - row.left() >= 2 * iconWidth + 26;
                if (visible) {
                    right -= iconWidth;
                    action->setGeometry(iconRect(right, row));
                }
                action->setVisible(visible);
            }
        }
    }

    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setBrush(underMouse() || isDown() ? fill.lighter(115) : fill);
        painter.setPen(hasFocus() ? QColor("#C8D8E2") : fill.lighter(140));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
        painter.fillRect(1, 1, 3, height() - 2, accent);

        const QRect row = firstRow();
        QRect label = row;
        if (row.width() >= iconWidth) {
            if (!taskIsTypeIcon || !taskAction) {
                typeIcon.paint(&painter, iconRect(row.left(), row).adjusted(2, 2, -2, -2));
            }
            label.adjust(iconWidth + 2, 0, 0, 0);
        }
        for (auto *action : {taskIsTypeIcon ? nullptr : taskAction, nodeAction}) {
            if (action && !action->isHidden()) {
                label.adjust(0, 0, -iconWidth, 0);
            }
        }
        if (important && label.width() >= 15) {
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(importantTaskPriorityColor));
            painter.drawEllipse(QRect(label.left(), row.center().y() - 3, 6, 6));
            painter.restore();
            label.adjust(9, 0, 0, 0);
        }

        const QFontMetrics metrics = fontMetrics();
        const auto drawLabel = [&](const QRect &area, const QString &text) {
            if (area.width() > 0) {
                painter.drawText(area, Qt::AlignVCenter | Qt::TextSingleLine,
                                 metrics.elidedText(text, Qt::ElideRight, area.width()));
            }
        };

        painter.setPen(QColor("#F1F4F6"));
        const bool showStartTime = compact() && label.width() >= metrics.horizontalAdvance(startTime) + 34;
        drawLabel(label, showStartTime ? startTime + "  " + title : title);

        if (!compact()) {
            painter.setPen(QColor("#CAD5DC"));
            QRect detail = row.translated(0, row.height());
            drawLabel(detail, time);
            if (!nodeName.isEmpty() && detail.bottom() + row.height() < height() - 1) {
                detail.translate(0, row.height());
                drawLabel(detail, nodeName);
            }
            if (!contextName.isEmpty() && detail.bottom() + row.height() < height() - 1) {
                detail.translate(0, row.height());
                if (!contextImage.isNull()) {
                    contextImage.paint(&painter, iconRect(detail.left(), detail).adjusted(2, 2, -2, -2));
                    detail.adjust(iconWidth, 0, 0, 0);
                }
                drawLabel(detail, contextName);
            }
        }
    }

private:
    static constexpr int iconWidth = 17;

    bool compact() const {
        return height() < 2 * fontMetrics().height() + 6;
    }

    QRect firstRow() const {
        const int textHeight = fontMetrics().height();
        return {8, compact() ? (height() - textHeight) / 2 : 3, width() - 13, textHeight};
    }

    static QRect iconRect(const int x, const QRect &row) {
        return {x, row.center().y() - iconWidth / 2, iconWidth, iconWidth};
    }

    QToolButton *createLink(const char *iconPath, const QString &name) {
        auto *button = new QToolButton(this);
        button->setIcon(QIcon(iconPath));
        button->setIconSize(QSize(13, 13));
        button->setAccessibleName(name);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(R"(
            QToolButton { background: transparent; border: 1px solid transparent; padding: 0; }
            QToolButton:hover { background: rgba(255, 255, 255, 38); }
            QToolButton:focus { border-color: #C8D8E2; }
        )");
        return button;
    }

    QString title;
    QString startTime;
    QString time;
    QString nodeName;
    QString contextName;
    QIcon contextImage;
    QIcon typeIcon;
    QColor fill;
    QColor accent;
    bool important;
    bool taskIsTypeIcon;
    QToolButton *taskAction;
    QToolButton *nodeAction;
};

} // namespace

class CalendarView::TimeGrid final : public QWidget {
public:
    TimeGrid(CalendarView &calendar, QScrollArea &scroll)
        : QWidget(&scroll), calendar(calendar), weekHeader(new QWidget(scroll.viewport())),
          currentTimeLine(new QWidget(this)), currentTimeLabel(new QLabel(this)) {
        weekHeader->setAttribute(Qt::WA_StyledBackground);
        weekHeader->setStyleSheet("background: #222222;");
        weekHeader->hide();
        for (QLabel *&day : weekDays) {
            day = new QLabel(weekHeader);
            day->setAlignment(Qt::AlignCenter);
        }
        currentTimeLine->setAttribute(Qt::WA_StyledBackground);
        currentTimeLine->setStyleSheet("background: #D27B5E;");
        currentTimeLine->setAttribute(Qt::WA_TransparentForMouseEvents);
        currentTimeLine->hide();
        currentTimeLabel->setStyleSheet(R"(
            background: #D27B5E; color: #1C1C1C; padding: 1px 4px; font-size: 11px; font-weight: 600;
        )");
        currentTimeLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        currentTimeLabel->hide();
    }

    void setRange(const QDate first, const int dayCount, const std::vector<CalendarEntry> &entries) {
        for (const auto &day : positionedDays) {
            for (const PositionedEntry &entry : day) {
                entry.button->hide();
                entry.button->deleteLater();
            }
        }

        firstDate = first;
        firstDayStart = first.startOfDay();
        positionedDays.clear();
        positionedDays.resize(dayCount);
        gridBottom = isWeek() ? gridTop() + 24 * hourHeight : timePosition(first.addDays(1).startOfDay());

        for (int column = 0; column < dayCount; ++column) {
            const QDate date = first.addDays(column);
            const QDateTime dayStart = date.startOfDay();
            const QDateTime dayEnd = date.addDays(1).startOfDay();
            auto &day = positionedDays[column];
            for (const CalendarEntry &entry : entries) {
                if (!occursOnDay(entry, dayStart, dayEnd)) {
                    continue;
                }

                const int start = timePosition(std::max(dayStart, entry.startAt));
                const int top = std::min(start, gridBottom - entryMinimumHeight);
                int end = top;
                if (entry.endAt) {
                    end = *entry.endAt >= dayEnd ? gridBottom : timePosition(*entry.endAt);
                }
                day.push_back({top, std::max(end, top + entryMinimumHeight),
                               calendar.createEntryButton(entry, this)});
            }
            assignColumns(day);

            if (isWeek()) {
                QLabel *label = weekDays[column];
                label->setText(german.toString(date, "ddd, d.M."));
                label->setStyleSheet(date == calendar.today
                    ? "background: #2B3740; color: #DBE6EE; border-left: 1px solid #373737;"
                    : "background: #222222; color: #ABABAB; border-left: 1px solid #373737;");
            }
        }

        setFixedHeight(gridBottom + gridPadding);
        positionItems();
        for (const auto &day : positionedDays) {
            for (const PositionedEntry &entry : day) {
                entry.button->show();
            }
        }
        weekHeader->setVisible(isWeek());
        weekHeader->raise();
        showCurrentTime(QDateTime::currentDateTime());
        update();
    }

    void showCurrentTime(const QDateTime &now) {
        const qint64 day = firstDate.daysTo(now.date());
        if (day < 0 || day >= static_cast<int>(positionedDays.size())) {
            currentTimeLine->hide();
            currentTimeLabel->hide();
            return;
        }

        const int x = dayBoundary(static_cast<int>(day));
        const int right = dayBoundary(static_cast<int>(day) + 1);
        const int y = timePosition(now);
        currentTimeLine->setGeometry(x, y, right - x, 2);
        currentTimeLine->show();
        currentTimeLine->raise();
        currentTimeLabel->setText(now.time().toString("HH:mm"));
        currentTimeLabel->adjustSize();
        currentTimeLabel->move(x, y + 1 - currentTimeLabel->height() / 2);
        currentTimeLabel->show();
        currentTimeLabel->raise();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.fillRect(rect(), QColor("#1C1C1C"));
        painter.fillRect(0, 0, timeGutterWidth, height(), QColor("#222222"));

        if (isWeek()) {
            const qint64 currentColumn = firstDate.daysTo(calendar.today);
            if (currentColumn >= 0 && currentColumn < daysPerWeek) {
                const int column = static_cast<int>(currentColumn);
                const int x = dayBoundary(column);
                painter.fillRect(x, 0, dayBoundary(column + 1) - x, height(), QColor("#20272D"));
            }
        }

        for (int hour = 0, y = gridTop(); y < gridBottom; ++hour, y += hourHeight) {
            const int halfHour = y + hourHeight / 2;
            painter.setPen(QColor("#373737"));
            painter.drawLine(timeGutterWidth, y, width(), y);
            if (halfHour < gridBottom) {
                painter.setPen(QColor("#2A2A2A"));
                painter.drawLine(timeGutterWidth, halfHour, width(), halfHour);
            }

            const QTime time = isWeek() ? QTime(hour, 0) : firstDayStart.addSecs(hour * 3600).time();
            painter.setPen(QColor("#ABABAB"));
            painter.drawText(0, y - gridPadding, timeGutterWidth - 10, 2 * gridPadding,
                             Qt::AlignRight | Qt::AlignVCenter, time.toString("HH:mm"));
        }

        painter.setPen(QColor("#373737"));
        painter.drawLine(timeGutterWidth, gridBottom, width(), gridBottom);
        for (int day = 0; day < static_cast<int>(positionedDays.size()); ++day) {
            const int x = dayBoundary(day);
            painter.drawLine(x, 0, x, height());
        }
    }

    void resizeEvent(QResizeEvent *) override {
        positionItems();
        showCurrentTime(QDateTime::currentDateTime());
    }

private:
    struct PositionedEntry {
        int top;
        int bottom;
        QAbstractButton *button;
        int column = 0;
        int columnCount = 1;
    };

    bool isWeek() const {
        return positionedDays.size() == daysPerWeek;
    }

    int gridTop() const {
        return gridPadding + (isWeek() ? weekdayHeaderHeight : 0);
    }

    int timePosition(const QDateTime &time) const {
        const qint64 milliseconds = isWeek()
            ? time.toLocalTime().time().msecsSinceStartOfDay()
            : firstDayStart.msecsTo(time);
        return gridTop() + static_cast<int>(milliseconds * hourHeight / millisecondsPerHour);
    }

    int dayBoundary(const int column) const {
        const int available = std::max(0, width() - timeGutterWidth);
        return timeGutterWidth + available * column / static_cast<int>(positionedDays.size());
    }

    static void assignColumns(std::vector<PositionedEntry> &entries) {
        std::ranges::sort(entries, [](const PositionedEntry &left, const PositionedEntry &right) {
            return left.top == right.top ? left.bottom > right.bottom : left.top < right.top;
        });

        for (auto first = entries.begin(); first != entries.end();) {
            // Connected overlaps share a column count, with columns reused as entries end.
            std::vector<int> columnEnds;
            int groupEnd = first->bottom;
            auto next = first;
            for (; next != entries.end() && next->top < groupEnd; ++next) {
                const auto column = std::ranges::find_if(columnEnds, [top = next->top](const int end) {
                    return end <= top;
                });
                next->column = static_cast<int>(column - columnEnds.begin());
                if (column == columnEnds.end()) {
                    columnEnds.push_back(next->bottom);
                } else {
                    *column = next->bottom;
                }
                groupEnd = std::max(groupEnd, next->bottom);
            }

            for (auto entry = first; entry != next; ++entry) {
                entry->columnCount = static_cast<int>(columnEnds.size());
            }
            first = next;
        }
    }

    void positionItems() {
        for (int day = 0; day < static_cast<int>(positionedDays.size()); ++day) {
            const int x = dayBoundary(day);
            const int dayWidth = dayBoundary(day + 1) - x;
            if (isWeek()) {
                weekDays[day]->setGeometry(x, 0, dayWidth, weekdayHeaderHeight);
            }

            const int margin = std::min(4, dayWidth / 6);
            const int availableWidth = dayWidth - 2 * margin;
            for (const PositionedEntry &entry : positionedDays[day]) {
                const int left = availableWidth * entry.column / entry.columnCount;
                const int right = availableWidth * (entry.column + 1) / entry.columnCount;
                const int columnWidth = right - left;
                entry.button->setGeometry(x + margin + left, entry.top,
                                          columnWidth - std::min(margin, columnWidth / 4),
                                          entry.bottom - entry.top);
            }
        }
        weekHeader->setGeometry(0, 0, width(), weekdayHeaderHeight);
    }

    CalendarView &calendar;
    QWidget *weekHeader;
    std::array<QLabel *, daysPerWeek> weekDays;
    QWidget *currentTimeLine;
    QLabel *currentTimeLabel;
    QDate firstDate;
    QDateTime firstDayStart;
    int gridBottom = 0;
    std::vector<std::vector<PositionedEntry>> positionedDays;
};

CalendarView::CalendarView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client), dateLabel(new QLabel(this)),
      viewButtons(new QButtonGroup(this)), pages(new QStackedWidget(this)),
      monthScroll(new QScrollArea(pages)) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(8);

    auto *header = new QHBoxLayout;
    header->setSpacing(4);
    dateLabel->setStyleSheet("color: #E5E5E5; font-size: 15px; font-weight: 600;");
    dateLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    header->addWidget(dateLabel, 1);

    auto *previous = new QToolButton(this);
    previous->setArrowType(Qt::LeftArrow);
    previous->setAccessibleName("Zurück");
    previous->setToolTip(previous->accessibleName());
    auto *next = new QToolButton(this);
    next->setArrowType(Qt::RightArrow);
    next->setAccessibleName("Weiter");
    next->setToolTip(next->accessibleName());
    auto *todayButton = new QToolButton(this);
    todayButton->setText("Heute");

    for (auto *button : {previous, todayButton, next}) {
        button->setMinimumSize(28, 28);
        button->setStyleSheet(toolButtonStyle);
        header->addWidget(button);
    }
    header->addSpacing(8);
    const std::array modeNames{"Tag", "Woche", "Monat"};
    for (int index = 0; index < static_cast<int>(modeNames.size()); ++index) {
        auto *button = new QToolButton(this);
        button->setText(modeNames[index]);
        button->setCheckable(true);
        button->setMinimumHeight(28);
        button->setStyleSheet(toolButtonStyle);
        viewButtons->addButton(button, index);
        header->addWidget(button);
    }
    layout->addLayout(header);

    auto *timeScroll = new QScrollArea(pages);
    for (auto *scroll : {timeScroll, monthScroll}) {
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setWidgetResizable(true);
        pages->addWidget(scroll);
    }
    grid = new TimeGrid(*this, *timeScroll);
    timeScroll->setWidget(grid);
    timeScroll->verticalScrollBar()->setSingleStep(hourHeight / 2);
    layout->addWidget(pages, 1);

    connect(previous, &QToolButton::clicked, this, [this] { navigate(-1); });
    connect(next, &QToolButton::clicked, this, [this] { navigate(1); });
    connect(todayButton, &QToolButton::clicked, this, [this] {
        showDate(QDate::currentDate(), viewMode);
    });
    connect(viewButtons, &QButtonGroup::idClicked, this, [this](const int id) {
        showDate(selectedDate, static_cast<ViewMode>(id));
    });

    showDate(today, ViewMode::Day);
    timeScroll->verticalScrollBar()->setValue(6 * hourHeight);

    auto *clock = new QTimer(this);
    clock->setSingleShot(true);
    clock->setTimerType(Qt::PreciseTimer);
    const auto updateClock = [this, clock] {
        const QDateTime now = QDateTime::currentDateTime();
        if (today != now.date()) {
            today = now.date();
            renderCalendar();
        } else if (viewMode != ViewMode::Month) {
            grid->showCurrentTime(now);
        }
        clock->start(60'000 - now.time().msecsSinceStartOfDay() % 60'000);
    };
    connect(clock, &QTimer::timeout, this, updateClock);
    updateClock();

    connect(&apiClient, &ApiClient::calendarEntriesReceived, this,
            [this](const std::vector<CalendarEntry> &receivedEntries) {
                entries = receivedEntries;
                renderCalendar();
            });
    connect(&apiClient, &ApiClient::taskReceived, this, [this](const std::vector<Task> &receivedTasks) {
        tasks = receivedTasks;
        renderCalendar();
    });
    connect(&apiClient, &ApiClient::contextsReceived, this, [this](const std::vector<Context> &receivedContexts) {
        contexts = receivedContexts;
        renderCalendar();
    });
    connect(&apiClient, &ApiClient::metadataReceived, this, &CalendarView::renderCalendar);
    const auto reloadEntries = [this] { apiClient.getCalendarEntries(); };
    connect(&apiClient, &ApiClient::calendarEntryCreated, this, reloadEntries);
    connect(&apiClient, &ApiClient::calendarEntryUpdated, this, reloadEntries);
    connect(&apiClient, &ApiClient::calendarEntryDeleted, this, reloadEntries);
    connect(&apiClient, &ApiClient::taskDeleted, this, reloadEntries);
    connect(&apiClient, &ApiClient::domainNodeDeleted, this, reloadEntries);
    connect(&apiClient, &ApiClient::routineExecutionsGenerated, this, reloadEntries);
    reloadEntries();
    apiClient.getTasks();
    apiClient.getContexts();
}

void CalendarView::showDate(const QDate date, const ViewMode mode) {
    selectedDate = date;
    viewMode = mode;
    viewButtons->button(static_cast<int>(mode))->setChecked(true);
    renderCalendar();
    if (mode == ViewMode::Month) {
        monthScroll->verticalScrollBar()->setValue(0);
    }
}

void CalendarView::navigate(const int step) {
    switch (viewMode) {
        case ViewMode::Day: showDate(selectedDate.addDays(step), viewMode); break;
        case ViewMode::Week: showDate(selectedDate.addDays(daysPerWeek * step), viewMode); break;
        case ViewMode::Month: showDate(selectedDate.addMonths(step), viewMode); break;
    }
}

void CalendarView::renderCalendar() {
    switch (viewMode) {
        case ViewMode::Day:
            dateLabel->setText(german.toString(selectedDate, "dddd, d. MMMM yyyy"));
            grid->setRange(selectedDate, 1, entries);
            break;
        case ViewMode::Week: {
            const QDate monday = mondayOf(selectedDate);
            const QDate sunday = monday.addDays(daysPerWeek - 1);
            dateLabel->setText(german.toString(monday, monday.year() == sunday.year() ? "dd.MM." : "dd.MM.yyyy")
                               + " – " + german.toString(sunday, "dd.MM.yyyy"));
            grid->setRange(monday, daysPerWeek, entries);
            break;
        }
        case ViewMode::Month:
            dateLabel->setText(german.toString(selectedDate, "MMMM yyyy"));
            renderMonth();
            break;
    }
    pages->setCurrentIndex(viewMode == ViewMode::Month ? 1 : 0);
}

void CalendarView::renderMonth() {
    const QDate firstOfMonth(selectedDate.year(), selectedDate.month(), 1);
    const QDate firstCell = mondayOf(firstOfMonth);
    const int cellCount = firstOfMonth.dayOfWeek() - 1 + firstOfMonth.daysInMonth();
    const int weekCount = (cellCount + daysPerWeek - 1) / daysPerWeek;

    auto *month = new QWidget;
    QPalette monthPalette = month->palette();
    monthPalette.setColor(QPalette::Window, QColor("#373737"));
    month->setPalette(monthPalette);

    auto *tiles = new QGridLayout(month);
    tiles->setContentsMargins(1, 1, 1, 1);
    tiles->setSpacing(1);
    for (int column = 0; column < daysPerWeek; ++column) {
        auto *label = new QLabel(german.dayName(column + 1, QLocale::ShortFormat), month);
        label->setAlignment(Qt::AlignCenter);
        label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        label->setFixedHeight(weekdayHeaderHeight);
        label->setStyleSheet("background: #222222; color: #ABABAB;");
        tiles->addWidget(label, 0, column);
        tiles->setColumnStretch(column, 1);
    }

    for (int week = 0; week < weekCount; ++week) {
        tiles->setRowStretch(week + 1, 1);
        for (int column = 0; column < daysPerWeek; ++column) {
            const QDate date = firstCell.addDays(week * daysPerWeek + column);
            const bool inMonth = date.month() == firstOfMonth.month();
            auto *cell = new QWidget(month);
            cell->setMinimumHeight(74);
            cell->setAutoFillBackground(true);
            QPalette cellPalette = cell->palette();
            cellPalette.setColor(QPalette::Window, QColor(date == today
                ? "#252E35" : inMonth ? "#1C1C1C" : "#202020"));
            cell->setPalette(cellPalette);
            tiles->addWidget(cell, week + 1, column);

            auto *items = new QVBoxLayout(cell);
            items->setContentsMargins(3, 3, 3, 3);
            items->setSpacing(2);

            auto *dateButton = new QToolButton(cell);
            dateButton->setText(QString::number(date.day()));
            dateButton->setAccessibleName(german.toString(date, "dddd, d. MMMM yyyy"));
            dateButton->setFixedSize(26, 23);
            dateButton->setStyleSheet(QString(R"(
                QToolButton { color: %1; background: transparent; border: 1px solid transparent; }
                QToolButton:hover { background: #333333; }
                QToolButton:focus { border-color: #777777; }
            )").arg(inMonth ? "#E5E5E5" : "#A0A0A0"));
            connect(dateButton, &QToolButton::clicked, this, [this, date] {
                showDate(date, ViewMode::Day);
            });
            items->addWidget(dateButton, 0, Qt::AlignLeft);

            const QDateTime start = date.startOfDay();
            const QDateTime end = date.addDays(1).startOfDay();
            for (const CalendarEntry &entry : entries) {
                if (occursOnDay(entry, start, end)) {
                    auto *button = createEntryButton(entry, cell);
                    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                    button->setFixedHeight(entryMinimumHeight);
                    items->addWidget(button);
                }
            }
            items->addStretch();
        }
    }

    const int scrollPosition = monthScroll->verticalScrollBar()->value();
    if (auto *previous = monthScroll->takeWidget()) {
        previous->deleteLater();
    }
    monthScroll->setWidget(month);
    monthScroll->verticalScrollBar()->setValue(scrollPosition);
}

QAbstractButton *CalendarView::createEntryButton(const CalendarEntry &entry, QWidget *parent) {
    const Task *task = findLinkedTask(entry);
    auto contextId = entry.contextId;
    if (!contextId && task && task->contextIds.size() == 1) {
        contextId = task->contextIds.front();
    }
    const auto context = contextId ? std::ranges::find(contexts, *contextId, &Context::id) : contexts.end();
    const bool important = task && importantTaskPriority(apiClient.metadata(), task->priorityId);
    auto *button = new CalendarEntryButton(entry, task, context == contexts.end() ? nullptr : &*context,
                                           important, parent);
    if (task) {
        connect(button->taskButton(), &QToolButton::clicked, this, [this, id = task->id] {
            emit openTaskRequested(id);
        });
        if (task->domainNodeId) {
            connect(button->nodeButton(), &QToolButton::clicked, this,
                    [this, id = *task->domainNodeId, title = task->domainNodeName] {
                        emit openDomainNodeRequested(id, title);
                    });
        }
    }
    connect(button, &QAbstractButton::clicked, this, [this, id = entry.id] {
        openEditDialog(id);
    });
    connect(button, &QWidget::customContextMenuRequested, this,
            [this, button, id = entry.id](const QPoint &point) {
                showEntryContextMenu(id, button->mapToGlobal(point));
            });
    return button;
}

void CalendarView::openCreateDialog() {
    QDateTime initialStart = QDateTime::currentDateTime();
    initialStart.setDate(selectedDate);
    const auto entry = showCalendarEntryDialog(this, "Kalendereintrag erstellen", {
        .startAt = initialStart,
        .endAt = initialStart.addSecs(3600),
        .isReminder = true
    }, "Erstellen", false);
    if (entry) {
        apiClient.createCalendarEntry(*entry);
    }
}

void CalendarView::refresh() {
    apiClient.generateRoutineExecutions();
}

void CalendarView::openEditDialog(const int id) {
    if (const auto *entry = findEntry(id)) {
        editCalendarEntry(this, apiClient, *entry);
    }
}

void CalendarView::showEntryContextMenu(const int id, const QPoint &position) {
    const CalendarEntry *entryPointer = findEntry(id);
    if (entryPointer == nullptr) {
        return;
    }
    const CalendarEntry entry = *entryPointer;

    QMenu menu(this);
    if (const Task *task = findLinkedTask(entry)) {
        menu.addAction(QIcon(taskIconPath), "Aufgabe öffnen", this, [this, id = task->id] {
            emit openTaskRequested(id);
        });
        if (task->domainNodeId) {
            menu.addAction(QIcon(nodeIconPath), "Aufschrieb öffnen", this,
                           [this, id = *task->domainNodeId, title = task->domainNodeName] {
                               emit openDomainNodeRequested(id, title);
                           });
        }
        menu.addSeparator();
    }
    QAction *editAction = menu.addAction(
        QIcon(":/assets/icons/edit.png"),
        "Bearbeiten"
    );
    editAction->setEnabled(!entry.isLocked);
    QAction *unlockAction = entry.isLocked ? menu.addAction("Entsperren") : nullptr;
    menu.addSeparator();
    QAction *deleteAction = menu.addAction(contextMenuDeleteIcon(), "Löschen");
    deleteAction->setEnabled(!entry.isLocked);

    QAction *selectedAction = menu.exec(position);
    if (selectedAction == editAction) {
        openEditDialog(id);
    } else if (selectedAction == deleteAction) {
        apiClient.deleteCalendarEntry(id);
    } else if (unlockAction && selectedAction == unlockAction) {
        CreateCalendarEntry updatedEntry = editableCalendarEntry(entry);
        updatedEntry.isLocked = false;
        apiClient.updateCalendarEntry(id, updatedEntry);
    }
}

const CalendarEntry *CalendarView::findEntry(const int id) const {
    for (const CalendarEntry &entry : entries) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

const Task *CalendarView::findLinkedTask(const CalendarEntry &entry) const {
    const auto id = entry.systemTaskId ? entry.systemTaskId : entry.routineSystemTaskId;
    const auto task = id ? std::ranges::find(tasks, *id, &Task::id) : tasks.end();
    return task == tasks.end() ? nullptr : &*task;
}
