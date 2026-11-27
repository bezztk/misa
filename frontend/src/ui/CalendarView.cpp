#include "CalendarView.h"

#include "../api/ApiClient.h"
#include "CalendarEntryDialog.h"
#include "ContextMenuStyle.h"

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
    explicit CalendarEntryButton(const CalendarEntry &entry, QWidget *parent)
        : QAbstractButton(parent),
          title(entry.name),
          time(entry.startAt.toLocalTime().toString("HH:mm")),
          fill(entry.routineExecutionId ? "#344934" : entry.systemTaskId ? "#354C60" : "#30485B"),
          accent(entry.isReminder() ? "#D39A55" : entry.routineExecutionId ? "#819D78" : "#7799B3") {
        const auto dateFormat = "dd.MM.yyyy, HH:mm";
        QString details = german.toString(entry.startAt.toLocalTime(), dateFormat);
        if (entry.endAt) {
            time += " – " + entry.endAt->toLocalTime().toString("HH:mm");
            details += " – " + german.toString(entry.endAt->toLocalTime(), dateFormat);
        }
        if (entry.isLocked) {
            details += " · Gesperrt";
        }

        setAccessibleName(title);
        setAccessibleDescription(details);
        setToolTip("<qt>" + title.toHtmlEscaped() + "<br>" + details + "</qt>");
        setAttribute(Qt::WA_Hover);
        setFocusPolicy(Qt::StrongFocus);
        setCursor(Qt::PointingHandCursor);
        setContextMenuPolicy(Qt::CustomContextMenu);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setBrush(underMouse() || isDown() ? fill.lighter(118) : fill);
        painter.setPen(hasFocus() ? QColor("#C8D8E2") : fill.lighter(140));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
        painter.fillRect(1, 1, 3, height() - 2, accent);

        const QRect textArea = rect().adjusted(9, 2, -5, -2);
        const QFontMetrics metrics = fontMetrics();
        const int textHeight = metrics.height();
        if (textArea.height() >= textHeight && textArea.width() > 0) {
            const int flags = Qt::AlignVCenter | Qt::TextSingleLine;
            painter.setPen(QColor("#F1F4F6"));
            if (textArea.height() >= textHeight * 2 + 4) {
                const QRect titleArea(textArea.x(), textArea.y() + 2, textArea.width(), textHeight);
                painter.drawText(titleArea, flags, metrics.elidedText(title, Qt::ElideRight, textArea.width()));
                painter.setPen(QColor("#CAD5DC"));
                painter.drawText(titleArea.translated(0, textHeight), flags,
                                 metrics.elidedText(time, Qt::ElideRight, textArea.width()));
            } else {
                const QString label = time.left(5) + "  " + title;
                painter.drawText(textArea, flags, metrics.elidedText(label, Qt::ElideRight, textArea.width()));
            }
        }
    }

private:
    QString title;
    QString time;
    QColor fill;
    QColor accent;
};

} // namespace

class CalendarView::TimeGrid final : public QWidget {
public:
    TimeGrid(CalendarView &calendar, QScrollArea &scroll)
        : QWidget(&scroll), calendar(calendar), weekHeader(new QWidget(scroll.viewport())),
          currentTimeLine(new QWidget(this)) {
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
            return;
        }

        const int x = dayBoundary(static_cast<int>(day));
        const int right = dayBoundary(static_cast<int>(day) + 1);
        currentTimeLine->setGeometry(x, timePosition(now), right - x, 2);
        currentTimeLine->show();
        currentTimeLine->raise();
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
    const auto reloadEntries = [this] { apiClient.getCalendarEntries(); };
    connect(&apiClient, &ApiClient::calendarEntryCreated, this, reloadEntries);
    connect(&apiClient, &ApiClient::calendarEntryUpdated, this, reloadEntries);
    connect(&apiClient, &ApiClient::calendarEntryDeleted, this, reloadEntries);
    connect(&apiClient, &ApiClient::taskDeleted, this, reloadEntries);
    connect(&apiClient, &ApiClient::routineExecutionsGenerated, this, reloadEntries);
    reloadEntries();
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
    auto *button = new CalendarEntryButton(entry, parent);
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
