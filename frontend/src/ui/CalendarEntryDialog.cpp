#include "CalendarEntryDialog.h"
#include "DateTimePicker.h"
#include "DialogUtils.h"

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
    layout->addRow(buttons);
    prepareDialog(dialog);

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
