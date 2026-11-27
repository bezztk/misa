#include "CalendarView.h"

#include "../api/ApiClient.h"
#include "CalendarEntryDialog.h"
#include "ContextMenuStyle.h"

#include <QAbstractButton>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int hourHeight = 68;
constexpr int timeGutterWidth = 64;
constexpr int gridPadding = 12;
constexpr int entryMinimumHeight = 20;
constexpr int millisecondsPerHour = 3'600'000;

const QLocale german(QLocale::German, QLocale::Germany);

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

class CalendarView::DayGrid final : public QWidget {
public:
    explicit DayGrid(CalendarView &calendar) : QWidget(&calendar), calendar(calendar) {}

    void setDay(const QDate date, const std::vector<CalendarEntry> &entries) {
        dayStart = date.startOfDay();
        dayEnd = date.addDays(1).startOfDay();
        const int gridBottom = timePosition(dayEnd);
        for (const PositionedEntry &entry : positionedEntries) {
            entry.button->hide();
            entry.button->deleteLater();
        }
        positionedEntries.clear();

        for (const CalendarEntry &entry : entries) {
            if (entry.startAt >= dayEnd
                || (entry.endAt ? *entry.endAt <= dayStart : entry.startAt < dayStart)) {
                continue;
            }

            const int start = timePosition(std::max(dayStart, entry.startAt));
            const int top = std::min(start, gridBottom - entryMinimumHeight);
            const int end = entry.endAt ? timePosition(std::min(dayEnd, *entry.endAt)) : top;
            const int bottom = std::max(end, top + entryMinimumHeight);

            auto *button = new CalendarEntryButton(entry, this);
            connect(button, &QAbstractButton::clicked, this, [this, id = entry.id] {
                calendar.openEditDialog(id);
            });
            connect(button, &QWidget::customContextMenuRequested, this,
                    [this, button, id = entry.id](const QPoint &point) {
                        calendar.showEntryContextMenu(id, button->mapToGlobal(point));
                    });
            positionedEntries.push_back({top, bottom, button});
        }

        assignColumns();
        setFixedHeight(gridBottom + gridPadding);
        positionButtons();
        for (const PositionedEntry &entry : positionedEntries) {
            entry.button->show();
        }
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        const int bottom = timePosition(dayEnd);
        painter.fillRect(rect(), QColor("#1C1C1C"));
        painter.fillRect(0, 0, timeGutterWidth, height(), QColor("#222222"));

        for (QDateTime hour = dayStart; hour < dayEnd; hour = hour.addSecs(3600)) {
            const int y = timePosition(hour);
            const int halfHour = y + hourHeight / 2;
            painter.setPen(QColor("#373737"));
            painter.drawLine(timeGutterWidth, y, width(), y);
            if (halfHour < bottom) {
                painter.setPen(QColor("#2A2A2A"));
                painter.drawLine(timeGutterWidth, halfHour, width(), halfHour);
            }

            painter.setPen(QColor("#ABABAB"));
            painter.drawText(0, y - gridPadding, timeGutterWidth - 10, 2 * gridPadding,
                             Qt::AlignRight | Qt::AlignVCenter, hour.toString("HH:mm"));
        }

        painter.setPen(QColor("#373737"));
        painter.drawLine(timeGutterWidth, bottom, width(), bottom);
        painter.drawLine(timeGutterWidth, 0, timeGutterWidth, height());
    }

    void resizeEvent(QResizeEvent *) override {
        positionButtons();
    }

private:
    struct PositionedEntry {
        int top;
        int bottom;
        CalendarEntryButton *button;
        int column = 0;
        int columnCount = 1;
    };

    int timePosition(const QDateTime &time) const {
        return gridPadding + static_cast<int>(dayStart.msecsTo(time) * hourHeight / millisecondsPerHour);
    }

    void assignColumns() {
        std::ranges::sort(positionedEntries, [](const PositionedEntry &left, const PositionedEntry &right) {
            return left.top == right.top ? left.bottom > right.bottom : left.top < right.top;
        });

        for (auto first = positionedEntries.begin(); first != positionedEntries.end();) {
            // Connected overlaps share a column count, with columns reused as entries end.
            std::vector<int> columnEnds;
            int groupEnd = first->bottom;
            auto next = first;
            for (; next != positionedEntries.end() && next->top < groupEnd; ++next) {
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

    void positionButtons() {
        constexpr int gap = 4;
        const int left = timeGutterWidth + gap;
        const int availableWidth = std::max(0, width() - left - gap);
        for (const PositionedEntry &entry : positionedEntries) {
            const int x = availableWidth * entry.column / entry.columnCount;
            const int nextX = availableWidth * (entry.column + 1) / entry.columnCount;
            const int columnWidth = nextX - x;
            entry.button->setGeometry(left + x, entry.top,
                                      columnWidth - std::min(gap, columnWidth / 4), entry.bottom - entry.top);
        }
    }

    CalendarView &calendar;
    QDateTime dayStart;
    QDateTime dayEnd;
    std::vector<PositionedEntry> positionedEntries;
};

CalendarView::CalendarView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client), dateLabel(new QLabel(this)), grid(new DayGrid(*this)) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(8);

    auto *header = new QHBoxLayout;
    header->setSpacing(4);
    dateLabel->setStyleSheet("color: #E5E5E5; font-size: 15px; font-weight: 600;");
    header->addWidget(dateLabel, 1);

    auto *previous = new QToolButton(this);
    previous->setArrowType(Qt::LeftArrow);
    previous->setAccessibleName("Vorheriger Tag");
    previous->setToolTip(previous->accessibleName());
    auto *next = new QToolButton(this);
    next->setArrowType(Qt::RightArrow);
    next->setAccessibleName("Nächster Tag");
    next->setToolTip(next->accessibleName());
    auto *today = new QToolButton(this);
    today->setText("Heute");

    for (auto *button : {previous, today, next}) {
        button->setMinimumSize(28, 28);
        button->setStyleSheet(R"(
            QToolButton {
                background: #242424;
                color: #E5E5E5;
                border: 1px solid #414141;
                border-radius: 0;
                padding: 0 6px;
            }
            QToolButton:hover, QToolButton:pressed { background: #333333; }
            QToolButton:focus { border-color: #777777; }
        )");
        header->addWidget(button);
    }
    layout->addLayout(header);

    connect(previous, &QToolButton::clicked, this, [this] { setDate(selectedDate.addDays(-1)); });
    connect(next, &QToolButton::clicked, this, [this] { setDate(selectedDate.addDays(1)); });
    connect(today, &QToolButton::clicked, this, [this] { setDate(QDate::currentDate()); });

    setDate(QDate::currentDate());
    auto *scroll = new QScrollArea(this);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidgetResizable(true);
    scroll->setWidget(grid);
    scroll->verticalScrollBar()->setSingleStep(hourHeight / 2);
    scroll->verticalScrollBar()->setValue(6 * hourHeight);
    layout->addWidget(scroll, 1);

    connect(&apiClient, &ApiClient::calendarEntriesReceived, this,
            [this](const std::vector<CalendarEntry> &receivedEntries) {
                entries = receivedEntries;
                renderDay();
            });
    const auto reloadEntries = [this] { apiClient.getCalendarEntries(); };
    connect(&apiClient, &ApiClient::calendarEntryCreated, this, reloadEntries);
    connect(&apiClient, &ApiClient::calendarEntryUpdated, this, reloadEntries);
    connect(&apiClient, &ApiClient::calendarEntryDeleted, this, reloadEntries);
    connect(&apiClient, &ApiClient::taskDeleted, this, reloadEntries);
    connect(&apiClient, &ApiClient::routineExecutionsGenerated, this, reloadEntries);
    reloadEntries();
}

void CalendarView::setDate(const QDate date) {
    selectedDate = date;
    dateLabel->setText(german.toString(date, "dddd, d. MMMM yyyy"));
    renderDay();
}

void CalendarView::renderDay() {
    grid->setDay(selectedDate, entries);
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
    QAction *lockAction = menu.addAction(
        entry.isLocked ? "Entsperren" : "Sperren"
    );
    menu.addSeparator();
    QAction *deleteAction = menu.addAction(contextMenuDeleteIcon(), "Löschen");
    deleteAction->setEnabled(!entry.isLocked);

    QAction *selectedAction = menu.exec(position);
    if (selectedAction == editAction) {
        openEditDialog(id);
    } else if (selectedAction == deleteAction) {
        apiClient.deleteCalendarEntry(id);
    } else if (selectedAction == lockAction) {
        CreateCalendarEntry updatedEntry = editableCalendarEntry(entry);
        updatedEntry.isLocked = !entry.isLocked;
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
