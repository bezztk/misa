#include "DateTimePicker.h"

#include "DialogUtils.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCalendarWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPalette>
#include <QStackedWidget>
#include <QTextCharFormat>
#include <QTimeZone>
#include <QToolButton>
#include <QVBoxLayout>

#include <optional>

namespace {

const QLocale german(QLocale::German, QLocale::Germany);

QString formatDateTime(const QDateTime &dateTime) {
    return german.toString(dateTime, "d. MMMM yyyy, HH:mm");
}

QString timeLabel(const int value) {
    return QString("%1").arg(value, 2, 10, QChar('0'));
}

QButtonGroup *createTimeGrid(QWidget *page, const QString &title, const int count, const int step) {
    constexpr int columns = 6;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(new QLabel(title, page));

    auto *grid = new QGridLayout;
    grid->setSpacing(6);
    layout->addLayout(grid);
    layout->addStretch();

    auto *buttons = new QButtonGroup(page);
    for (int index = 0; index < count; ++index) {
        const int value = index * step;
        auto *button = new QToolButton(page);
        button->setText(timeLabel(value));
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setFixedSize(38, 32);
        button->setCheckable(true);
        button->setFocusPolicy(Qt::StrongFocus);
        grid->addWidget(button, index / columns, index % columns);
        buttons->addButton(button, value);
    }
    return buttons;
}

std::optional<QDateTime> selectDateTime(
    QWidget *parent,
    const QString &title,
    const QDateTime &initial,
    const QDateTime &minimum
) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setLocale(german);

    QPalette palette = dialog.palette();
    palette.setColor(QPalette::Window, QColor("#242424"));
    palette.setColor(QPalette::WindowText, QColor("#F4F4F4"));
    palette.setColor(QPalette::Base, QColor("#242424"));
    palette.setColor(QPalette::Text, QColor("#F4F4F4"));
    palette.setColor(QPalette::Button, QColor("#303030"));
    palette.setColor(QPalette::ButtonText, QColor("#F4F4F4"));
    palette.setColor(QPalette::Highlight, QColor("#505050"));
    palette.setColor(QPalette::HighlightedText, QColor("#F4F4F4"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#999999"));
    dialog.setPalette(palette);

    auto *layout = new QVBoxLayout(&dialog);
    auto *selection = new QHBoxLayout;
    selection->setSpacing(20);
    layout->addLayout(selection);

    auto *calendar = new QCalendarWidget(&dialog);
    calendar->setMinimumHeight(220);
    calendar->setFirstDayOfWeek(Qt::Monday);
    calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    if (minimum.isValid()) {
        calendar->setMinimumDate(minimum.date());
    }
    calendar->setSelectedDate(initial.date());
    calendar->setStyleSheet(R"(
        QCalendarWidget QWidget#qt_calendar_navigationbar {
            background: #303030;
        }
        QCalendarWidget QToolButton {
            color: #F4F4F4;
            background: transparent;
        }
        QCalendarWidget QToolButton:hover {
            background: #414141;
        }
        QCalendarWidget QAbstractItemView {
            background: #242424;
            selection-background-color: #505050;
            selection-color: #F4F4F4;
        }
    )");
    QTextCharFormat weekend;
    weekend.setForeground(QColor("#B8B8B8"));
    calendar->setWeekdayTextFormat(Qt::Saturday, weekend);
    calendar->setWeekdayTextFormat(Qt::Sunday, weekend);
    selection->addWidget(calendar, 1);

    auto *timePanel = new QWidget(&dialog);
    timePanel->setStyleSheet(R"(
        QToolButton, QLineEdit {
            color: #F4F4F4;
            background: #303030;
            border: 1px solid #505050;
            border-radius: 4px;
        }
        QToolButton:hover {
            background: #414141;
        }
        QToolButton:checked {
            color: #242424;
            background: #B8B8B8;
            border-color: #B8B8B8;
        }
        QToolButton:focus, QLineEdit:focus {
            border-color: #F4F4F4;
        }
        QToolButton#timeHour, QLineEdit, QLabel#timeSeparator {
            font-size: 19px;
            font-weight: 600;
        }
    )");
    selection->addWidget(timePanel, 0, Qt::AlignTop);

    auto *timeLayout = new QVBoxLayout(timePanel);
    timeLayout->setContentsMargins(0, 0, 0, 0);
    timeLayout->setSpacing(10);
    auto *timeValue = new QHBoxLayout;
    timeValue->setSpacing(4);
    timeLayout->addLayout(timeValue);
    timeValue->addStretch();

    auto *hours = new QToolButton(timePanel);
    hours->setObjectName("timeHour");
    hours->setAccessibleName("Stunde auswählen");
    hours->setText(timeLabel(initial.time().hour()));
    hours->setToolButtonStyle(Qt::ToolButtonTextOnly);
    hours->setFixedSize(48, 34);
    timeValue->addWidget(hours);

    auto *separator = new QLabel(":", timePanel);
    separator->setObjectName("timeSeparator");
    timeValue->addWidget(separator);

    auto *minutes = new QLineEdit(timeLabel(initial.time().minute()), timePanel);
    minutes->setAccessibleName("Minuten");
    minutes->setValidator(new QIntValidator(0, 59, minutes));
    minutes->setMaxLength(2);
    minutes->setAlignment(Qt::AlignCenter);
    minutes->setFixedSize(48, 34);
    timeValue->addWidget(minutes);
    timeValue->addStretch();

    auto *timePages = new QStackedWidget(timePanel);
    timeLayout->addWidget(timePages);
    auto *hourPage = new QWidget(timePages);
    auto *hourButtons = createTimeGrid(hourPage, "Stunde wählen", 24, 1);
    timePages->addWidget(hourPage);
    hourButtons->button(initial.time().hour())->setChecked(true);

    auto *minutePage = new QWidget(timePages);
    auto *minuteButtons = createTimeGrid(minutePage, "Minute wählen", 12, 5);
    timePages->addWidget(minutePage);

    const auto markMinute = [minuteButtons](const QString &text) {
        bool valid = false;
        const int minute = text.toInt(&valid);
        minuteButtons->setExclusive(false);
        for (auto *button : minuteButtons->buttons()) {
            button->setChecked(valid && minuteButtons->id(button) == minute);
        }
        minuteButtons->setExclusive(true);
    };
    QObject::connect(minutes, &QLineEdit::textChanged, &dialog, markMinute);
    markMinute(minutes->text());
    QObject::connect(minutes, &QLineEdit::editingFinished, &dialog, [minutes] {
        minutes->setText(timeLabel(minutes->text().toInt()));
    });
    QObject::connect(
        qApp, &QApplication::focusChanged, &dialog,
        [minutes, timePages, minutePage](QWidget *, QWidget *current) {
            if (current == minutes) {
                timePages->setCurrentWidget(minutePage);
            }
        }
    );
    QObject::connect(hours, &QToolButton::clicked, &dialog, [timePages, hourPage, hourButtons] {
        timePages->setCurrentWidget(hourPage);
        hourButtons->checkedButton()->setFocus();
    });
    QObject::connect(minuteButtons, &QButtonGroup::idClicked, &dialog, [minutes](const int minute) {
        minutes->setText(timeLabel(minute));
    });

    auto *preview = new QLabel(&dialog);
    preview->setWordWrap(true);
    preview->setStyleSheet("color: #F4F4F4; background: #303030; padding: 8px 12px;");
    layout->addWidget(preview);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog
    );
    auto *accept = buttons->button(QDialogButtonBox::Ok);
    accept->setText("Übernehmen");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    QDateTime selected;
    const auto updatePreview = [&] {
        if (!minutes->hasAcceptableInput()) {
            preview->setText("Minuten von 00 bis 59 eingeben.");
            accept->setEnabled(false);
            return;
        }

        const QDate date = calendar->selectedDate();
        const QTime time(hourButtons->checkedId(), minutes->text().toInt());
        const QTime initialTime = initial.time();
        const bool unchanged = date == initial.date()
            && time == QTime(initialTime.hour(), initialTime.minute());
        selected = unchanged ? initial : QDateTime(date, time, initial.timeZone());

        const bool validTime = unchanged
            || (selected.isValid() && selected.date() == date && selected.time() == time);
        accept->setEnabled(validTime && (!minimum.isValid() || selected >= minimum));
        if (!validTime) {
            preview->setText("Diese Uhrzeit ist an diesem Datum nicht verfügbar.");
        } else if (!accept->isEnabled()) {
            preview->setText("Frühester Zeitpunkt: " + german.toString(minimum, "d. MMMM yyyy, HH:mm:ss"));
        } else {
            preview->setText(formatDateTime(selected));
        }
    };
    QObject::connect(calendar, &QCalendarWidget::selectionChanged, &dialog, updatePreview);
    QObject::connect(
        hourButtons, &QButtonGroup::idClicked, &dialog,
        [hours, timePages, minutePage, minutes, updatePreview](const int hour) {
            hours->setText(timeLabel(hour));
            timePages->setCurrentWidget(minutePage);
            minutes->setFocus();
            minutes->selectAll();
            updatePreview();
        }
    );
    QObject::connect(minutes, &QLineEdit::textChanged, &dialog, updatePreview);
    updatePreview();
    prepareDialog(dialog, 620);
    calendar->setFocus();

    if (dialog.exec() != QDialog::Accepted) {
        return std::nullopt;
    }
    return selected;
}

} // namespace

DateTimeButton::DateTimeButton(
    const QString &title, const QDateTime &dateTime, QWidget *parent
) : QPushButton(parent) {
    setAccessibleName(title);
    setObjectName("dateTimeField");
    setCursor(Qt::PointingHandCursor);
    setIcon(QIcon(":/assets/icons/calendar.png"));
    setIconSize(QSize(16, 16));
    setStyleSheet(R"(
        QPushButton#dateTimeField {
            min-height: 36px;
            padding: 0 12px;
            text-align: left;
            font-family: "Bahnschrift";
            font-size: 14px;
            font-weight: 500;
            color: #F4F4F4;
            background: #2B2B2B;
            border: 1px solid #5E5E5E;
            border-radius: 0;
        }
        QPushButton#dateTimeField:hover {
            background: #353535;
        }
        QPushButton#dateTimeField:pressed {
            background: #414141;
        }
        QPushButton#dateTimeField:focus {
            border-color: #B8B8B8;
        }
    )");
    setDateTime(dateTime);
    setAutoDefault(false);
    connect(this, &QPushButton::clicked, this, [this, title] {
        const QDateTime now = QDateTime::currentDateTime();
        const QDateTime currentMinute = now.addMSecs(-(now.time().msecsSinceStartOfDay() % 60000));
        const QDateTime initial = value.isValid() ? value : currentMinute;
        if (const auto selected = selectDateTime(this, title, initial, minimumDateTime)) {
            setDateTime(*selected);
        }
    });
}

QDateTime DateTimeButton::dateTime() const {
    return value;
}

void DateTimeButton::setDateTime(const QDateTime &dateTime) {
    const bool changed = value != dateTime;
    value = dateTime;
    setText(value.isValid() ? value.toString("dd.MM.yyyy   │   HH:mm") : "Datum und Uhrzeit auswählen");
    setAccessibleDescription(value.isValid() ? value.toString("dd.MM.yyyy, HH:mm") : QString());
    if (changed) {
        emit dateTimeChanged();
    }
}

void DateTimeButton::setMinimumDateTime(const QDateTime &minimum) {
    minimumDateTime = minimum;
    if (value < minimumDateTime) {
        setDateTime(minimumDateTime);
    }
}
