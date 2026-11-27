#include "DateTimePicker.h"

#include "DialogUtils.h"

#include <QCalendarWidget>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPalette>
#include <QTextCharFormat>
#include <QTimeZone>
#include <QVBoxLayout>

#include <optional>

namespace {

const QLocale german(QLocale::German, QLocale::Germany);

QString formatDateTime(const QDateTime &dateTime) {
    return german.toString(dateTime, "d. MMMM yyyy, HH:mm");
}

QComboBox *createTimeSelector(QWidget *parent, const int maximum, const int initialValue) {
    auto *selector = new QComboBox(parent);
    selector->setEditable(true);
    selector->setInsertPolicy(QComboBox::NoInsert);
    selector->setMaxVisibleItems(10);
    for (int value = 0; value <= maximum; ++value) {
        selector->addItem(QString("%1").arg(value, 2, 10, QChar('0')));
    }
    selector->setValidator(new QIntValidator(0, maximum, selector));
    selector->lineEdit()->setMaxLength(2);
    selector->setCurrentIndex(initialValue);
    return selector;
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

    auto *timeLayout = new QFormLayout;
    selection->addLayout(timeLayout);
    auto *hours = createTimeSelector(&dialog, 23, initial.time().hour());
    auto *minutes = createTimeSelector(&dialog, 59, initial.time().minute());
    hours->setMinimumWidth(72);
    minutes->setMinimumWidth(72);
    timeLayout->addRow("&Stunden", hours);
    timeLayout->addRow("&Minuten", minutes);

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
        if (!hours->lineEdit()->hasAcceptableInput() || !minutes->lineEdit()->hasAcceptableInput()) {
            preview->setText("Stunden und Minuten eingeben.");
            accept->setEnabled(false);
            return;
        }

        const QDate date = calendar->selectedDate();
        const QTime time(hours->currentText().toInt(), minutes->currentText().toInt());
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
    QObject::connect(hours, &QComboBox::currentTextChanged, &dialog, updatePreview);
    QObject::connect(minutes, &QComboBox::currentTextChanged, &dialog, updatePreview);
    updatePreview();
    prepareDialog(dialog, 520);
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
    setText(value.isValid() ? value.toString("dd.MM.yyyy HH:mm") : "Datum und Uhrzeit auswählen");
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
