#include "RoutineDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QTimeEdit>
#include <QTimeZone>

#include <vector>

static bool parseNumberList(
    const QString &text,
    const int minimum,
    const int maximum,
    QString &normalized
) {
    if (text.trimmed().isEmpty()) {
        normalized.clear();
        return true;
    }

    QStringList values;
    for (const QString &part : text.split(',')) {
        bool valid;
        const int value = part.trimmed().toInt(&valid);
        if (!valid || value < minimum || value > maximum) {
            return false;
        }
        values.append(QString::number(value));
    }
    normalized = values.join(',');
    return true;
}

static QString rruleValue(const QString &rule, const QString &key) {
    for (const QString &part : rule.split(';')) {
        if (part.startsWith(key + '=')) {
            return part.mid(key.size() + 1);
        }
    }
    return {};
}

bool showRoutineDialog(
    QWidget *parent,
    const QString &title,
    SaveRoutine &routine,
    const std::optional<Routine> existing
) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(
        existing.has_value() ? existing->name : routine.name,
        &dialog
    );
    layout->addRow("Name", nameEdit);

    auto *startDateEdit = new QDateEdit(QDate::currentDate(), &dialog);
    startDateEdit->setCalendarPopup(true);
    startDateEdit->setDisplayFormat("dd.MM.yyyy");
    layout->addRow("Startdatum", startDateEdit);

    auto *fixedTimeEdit = new QCheckBox("Feste Uhrzeit", &dialog);
    auto *timeEdit = new QTimeEdit(QTime(9, 0), &dialog);
    timeEdit->setDisplayFormat("HH:mm");
    auto *timeRow = new QWidget(&dialog);
    auto *timeLayout = new QHBoxLayout(timeRow);
    timeLayout->setContentsMargins(0, 0, 0, 0);
    timeLayout->addWidget(fixedTimeEdit);
    timeLayout->addWidget(timeEdit);
    timeLayout->addStretch();
    layout->addRow("Uhrzeit", timeRow);

    auto *frequencyEdit = new QComboBox(&dialog);
    frequencyEdit->addItem("Täglich", "DAILY");
    frequencyEdit->addItem("Wöchentlich", "WEEKLY");
    frequencyEdit->addItem("Monatlich", "MONTHLY");
    frequencyEdit->addItem("Jährlich", "YEARLY");
    layout->addRow("Häufigkeit", frequencyEdit);

    auto *intervalEdit = new QSpinBox(&dialog);
    intervalEdit->setRange(1, 999);
    layout->addRow("Intervall", intervalEdit);

    const QStringList dayLabels{"Mo", "Di", "Mi", "Do", "Fr", "Sa", "So"};
    const QStringList dayValues{"MO", "TU", "WE", "TH", "FR", "SA", "SU"};
    std::vector<QCheckBox *> dayEdits;
    auto *daysRow = new QWidget(&dialog);
    auto *daysLayout = new QHBoxLayout(daysRow);
    daysLayout->setContentsMargins(0, 0, 0, 0);
    for (const QString &label : dayLabels) {
        auto *dayEdit = new QCheckBox(label, daysRow);
        dayEdits.push_back(dayEdit);
        daysLayout->addWidget(dayEdit);
    }
    layout->addRow("Wochentage", daysRow);

    auto *monthDaysEdit = new QLineEdit(&dialog);
    monthDaysEdit->setPlaceholderText("z. B. 1,15,31");
    layout->addRow("Monatstage", monthDaysEdit);
    auto *monthsEdit = new QLineEdit(&dialog);
    monthsEdit->setPlaceholderText("z. B. 1,6,12");
    layout->addRow("Monate", monthsEdit);

    auto *countEnabled = new QCheckBox("Begrenzen", &dialog);
    auto *countEdit = new QSpinBox(&dialog);
    countEdit->setRange(1, 100000);
    auto *countRow = new QWidget(&dialog);
    auto *countLayout = new QHBoxLayout(countRow);
    countLayout->setContentsMargins(0, 0, 0, 0);
    countLayout->addWidget(countEnabled);
    countLayout->addWidget(countEdit);
    layout->addRow("Anzahl", countRow);

    auto *untilEnabled = new QCheckBox("Begrenzen", &dialog);
    auto *untilEdit = new QDateEdit(QDate::currentDate().addYears(1), &dialog);
    untilEdit->setCalendarPopup(true);
    untilEdit->setDisplayFormat("dd.MM.yyyy");
    auto *untilRow = new QWidget(&dialog);
    auto *untilLayout = new QHBoxLayout(untilRow);
    untilLayout->setContentsMargins(0, 0, 0, 0);
    untilLayout->addWidget(untilEnabled);
    untilLayout->addWidget(untilEdit);
    layout->addRow("Enddatum", untilRow);

    auto *durationEnabled = new QCheckBox("Dauer verwenden", &dialog);
    durationEnabled->setChecked(!existing.has_value() || existing->estimatedDuration.has_value());
    auto *durationEdit = new QSpinBox(&dialog);
    durationEdit->setRange(1, 1440);
    durationEdit->setSuffix(" Minuten");
    durationEdit->setValue(
        existing.has_value() && existing->estimatedDuration.has_value()
            ? *existing->estimatedDuration : 30
    );
    auto *durationRow = new QWidget(&dialog);
    auto *durationLayout = new QHBoxLayout(durationRow);
    durationLayout->setContentsMargins(0, 0, 0, 0);
    durationLayout->addWidget(durationEnabled);
    durationLayout->addWidget(durationEdit);
    layout->addRow("Geschätzte Dauer", durationRow);

    auto *optionalEdit = new QCheckBox(&dialog);
    optionalEdit->setChecked(existing.has_value() && existing->isOptional);
    layout->addRow("Optional", optionalEdit);
    auto *activeEdit = new QCheckBox(&dialog);
    activeEdit->setChecked(!existing.has_value() || existing->isActive);
    layout->addRow("Aktiv", activeEdit);

    QObject::connect(fixedTimeEdit, &QCheckBox::toggled, timeEdit, &QWidget::setEnabled);
    QObject::connect(durationEnabled, &QCheckBox::toggled,
                     durationEdit, &QWidget::setEnabled);
    QObject::connect(countEnabled, &QCheckBox::toggled, &dialog,
                     [untilEnabled](const bool checked) {
                         if (checked) untilEnabled->setChecked(false);
                     });
    QObject::connect(untilEnabled, &QCheckBox::toggled, &dialog,
                     [countEnabled](const bool checked) {
                         if (checked) countEnabled->setChecked(false);
                     });

    if (existing.has_value()) {
        const QStringList lines = existing->recurrenceRuleSet.split('\n');
        if (lines.size() == 2) {
            const QString start = lines[0].trimmed();
            if (start.startsWith("DTSTART;VALUE=DATE:")) {
                startDateEdit->setDate(QDate::fromString(start.section(':', 1), "yyyyMMdd"));
            } else {
                const QString value = start.section(':', 1);
                startDateEdit->setDate(QDate::fromString(value.left(8), "yyyyMMdd"));
                timeEdit->setTime(QTime::fromString(value.mid(9, 6), "HHmmss"));
                fixedTimeEdit->setChecked(true);
            }

            const QString rule = lines[1].trimmed().mid(QString("RRULE:").size());
            frequencyEdit->setCurrentIndex(
                frequencyEdit->findData(rruleValue(rule, "FREQ"))
            );
            const QString interval = rruleValue(rule, "INTERVAL");
            intervalEdit->setValue(interval.isEmpty() ? 1 : interval.toInt());
            const QStringList selectedDays = rruleValue(rule, "BYDAY").split(',');
            for (int index = 0; index < dayValues.size(); ++index) {
                dayEdits[index]->setChecked(selectedDays.contains(dayValues[index]));
            }
            monthDaysEdit->setText(rruleValue(rule, "BYMONTHDAY"));
            monthsEdit->setText(rruleValue(rule, "BYMONTH"));
            const QString count = rruleValue(rule, "COUNT");
            if (!count.isEmpty()) {
                countEnabled->setChecked(true);
                countEdit->setValue(count.toInt());
            }
            const QString until = rruleValue(rule, "UNTIL");
            if (!until.isEmpty()) {
                untilEnabled->setChecked(true);
                if (until.endsWith('Z')) {
                    QDateTime untilDateTime = QDateTime::fromString(
                        until, "yyyyMMdd'T'HHmmss'Z'"
                    );
                    untilDateTime.setTimeZone(QTimeZone::UTC);
                    untilEdit->setDate(
                        untilDateTime.toTimeZone(QTimeZone("Europe/Berlin")).date()
                    );
                } else {
                    untilEdit->setDate(QDate::fromString(until, "yyyyMMdd"));
                }
            }
        }
    }
    timeEdit->setEnabled(fixedTimeEdit->isChecked());
    durationEdit->setEnabled(durationEnabled->isChecked());

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    while (dialog.exec() == QDialog::Accepted) {
        QString monthDays;
        QString months;
        if (nameEdit->text().trimmed().isEmpty()
            || !parseNumberList(monthDaysEdit->text(), 1, 31, monthDays)
            || !parseNumberList(monthsEdit->text(), 1, 12, months)) {
            QMessageBox::warning(&dialog, title, "Bitte gültige Werte eingeben.");
            continue;
        }

        const bool fixedTime = fixedTimeEdit->isChecked();
        const QString startLine = fixedTime
            ? "DTSTART;TZID=Europe/Berlin:"
                + startDateEdit->date().toString("yyyyMMdd") + 'T'
                + timeEdit->time().toString("HHmmss")
            : "DTSTART;VALUE=DATE:" + startDateEdit->date().toString("yyyyMMdd");

        QStringList ruleParts{
            "FREQ=" + frequencyEdit->currentData().toString(),
            "INTERVAL=" + QString::number(intervalEdit->value())
        };
        QStringList selectedDays;
        for (int index = 0; index < dayValues.size(); ++index) {
            if (dayEdits[index]->isChecked()) selectedDays.append(dayValues[index]);
        }
        if (!selectedDays.isEmpty()) ruleParts.append("BYDAY=" + selectedDays.join(','));
        if (!monthDays.isEmpty()) ruleParts.append("BYMONTHDAY=" + monthDays);
        if (!months.isEmpty()) ruleParts.append("BYMONTH=" + months);
        if (fixedTime) {
            ruleParts.append("BYHOUR=" + QString::number(timeEdit->time().hour()));
            ruleParts.append("BYMINUTE=" + QString::number(timeEdit->time().minute()));
        }
        if (countEnabled->isChecked()) {
            ruleParts.append("COUNT=" + QString::number(countEdit->value()));
        } else if (untilEnabled->isChecked()) {
            if (fixedTime) {
                const QDateTime localUntil(
                    untilEdit->date(), QTime(23, 59, 59), QTimeZone("Europe/Berlin")
                );
                ruleParts.append(
                    "UNTIL=" + localUntil.toUTC().toString("yyyyMMdd'T'HHmmss'Z'")
                );
            } else {
                ruleParts.append("UNTIL=" + untilEdit->date().toString("yyyyMMdd"));
            }
        }

        routine = SaveRoutine{
            .name = nameEdit->text().trimmed(),
            .recurrenceRuleSet = startLine + "\r\nRRULE:" + ruleParts.join(';'),
            .timezone = "Europe/Berlin",
            .estimatedDuration = durationEnabled->isChecked()
                ? std::optional<int>(durationEdit->value()) : std::nullopt,
            .isOptional = optionalEdit->isChecked(),
            .isActive = activeEdit->isChecked()
        };
        return true;
    }
    return false;
}
