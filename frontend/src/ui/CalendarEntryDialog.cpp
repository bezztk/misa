#include "CalendarEntryDialog.h"
#include "DateTimePicker.h"
#include "DialogUtils.h"
#include "../models/Task.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

std::optional<CreateCalendarEntry> showCalendarEntryDialog(
    QWidget *parent,
    const QString &title,
    const CreateCalendarEntry &initialEntry,
    const QString &acceptText,
    const bool acceptOnOutsideClick
) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    auto *layout = new QFormLayout(&dialog);

    auto *typeEdit = new QComboBox(&dialog);
    typeEdit->addItem("Erinnerung", true);
    typeEdit->addItem("Termin", false);
    typeEdit->setCurrentIndex(initialEntry.isReminder ? 0 : 1);
    layout->addRow("Art", typeEdit);

    auto *nameEdit = new QLineEdit(initialEntry.name, &dialog);
    layout->addRow("Name", nameEdit);

    auto *startEdit = new DateTimeButton("Start auswählen", initialEntry.startAt, &dialog);
    layout->addRow("Start", startEdit);

    auto *endLabel = new QLabel("Ende", &dialog);
    auto *endEdit = new DateTimeButton(
        "Ende auswählen",
        initialEntry.endAt.value_or(initialEntry.startAt.addSecs(3600)),
        &dialog
    );
    layout->addRow(endLabel, endEdit);

    const auto updateEndRange = [startEdit, endEdit] {
        endEdit->setMinimumDateTime(startEdit->dateTime().addSecs(60));
    };
    const auto updateType = [typeEdit, endLabel, endEdit] {
        const bool isReminder = typeEdit->currentData().toBool();
        endLabel->setVisible(!isReminder);
        endEdit->setVisible(!isReminder);
    };
    QObject::connect(
        startEdit,
        &DateTimeButton::dateTimeChanged,
        &dialog,
        updateEndRange
    );
    QObject::connect(
        typeEdit,
        &QComboBox::currentIndexChanged,
        &dialog,
        updateType
    );
    updateEndRange();
    updateType();

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    buttons->button(QDialogButtonBox::Save)->setText(acceptText);
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    prepareFormDialog(dialog, *buttons);

    if (execDialog(dialog, acceptOnOutsideClick) != QDialog::Accepted
        || nameEdit->text().trimmed().isEmpty()) {
        return std::nullopt;
    }

    const bool isReminder = typeEdit->currentData().toBool();
    CreateCalendarEntry result{
        .name = nameEdit->text().trimmed(),
        .startAt = startEdit->dateTime(),
        .isReminder = isReminder,
        .systemTaskId = initialEntry.systemTaskId
    };
    if (!isReminder) {
        result.endAt = endEdit->dateTime();
    }
    return result;
}

std::optional<CreateCalendarEntry> showTaskScheduleDialog(QWidget *parent, const Task &task, const bool isReminder) {
    QDialog dialog(parent);
    dialog.setWindowTitle(isReminder ? "Erinnerung einplanen" : "Termin einplanen");
    auto *layout = new QFormLayout(&dialog);

    auto *startEdit = new DateTimeButton(isReminder ? "Zeitpunkt auswählen" : "Beginn auswählen", {}, &dialog);
    layout->addRow(isReminder ? "Wann soll erinnert werden?" : "Beginn", startEdit);

    auto *endEdit = isReminder ? nullptr : new DateTimeButton("Ende auswählen", {}, &dialog);
    if (endEdit) {
        layout->addRow("Ende", endEdit);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    auto *save = buttons->button(QDialogButtonBox::Save);
    save->setText("Einplanen");
    save->setEnabled(false);
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    const auto updateSave = [startEdit, endEdit, save] {
        save->setEnabled(startEdit->dateTime().isValid() && (!endEdit || endEdit->dateTime().isValid()));
    };
    QObject::connect(startEdit, &DateTimeButton::dateTimeChanged, &dialog, [startEdit, endEdit, updateSave] {
        if (endEdit) {
            const QDateTime minimumEnd = startEdit->dateTime().addSecs(60);
            if (endEdit->dateTime().isValid() && endEdit->dateTime() < minimumEnd) {
                endEdit->setDateTime({});
            }
            endEdit->setMinimumDateTime(minimumEnd);
        }
        updateSave();
    });
    if (endEdit) {
        QObject::connect(endEdit, &DateTimeButton::dateTimeChanged, &dialog, updateSave);
    }

    prepareFormDialog(dialog, *buttons);
    if (execDialog(dialog) != QDialog::Accepted) {
        return std::nullopt;
    }

    return CreateCalendarEntry{
        .name = task.name,
        .startAt = startEdit->dateTime(),
        .endAt = endEdit ? std::optional(endEdit->dateTime()) : std::nullopt,
        .isReminder = isReminder,
        .systemTaskId = task.id
    };
}
