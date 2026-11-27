#include "CalendarEntryDialog.h"
#include "ContextMenuStyle.h"
#include "DateTimePicker.h"
#include "DialogUtils.h"
#include "../api/ApiClient.h"
#include "../models/Task.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

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

CreateCalendarEntry editableCalendarEntry(const CalendarEntry &entry) {
    return {
        .name = entry.name,
        .startAt = entry.startAt.toLocalTime(),
        .endAt = entry.endAt.has_value() ? std::optional(entry.endAt->toLocalTime()) : std::nullopt,
        .isReminder = entry.isReminder(),
        .systemTaskId = entry.systemTaskId,
        .isLocked = entry.isLocked
    };
}

void editCalendarEntry(QWidget *parent, ApiClient &apiClient, const CalendarEntry entry) {
    if (entry.isLocked) {
        return;
    }
    if (const auto updated = showCalendarEntryDialog(
            parent, "Kalendereintrag bearbeiten", editableCalendarEntry(entry), "Fertig", true)) {
        apiClient.updateCalendarEntry(entry.id, *updated);
    }
}

static QString planningHeading(const CalendarEntry &entry, const bool past) {
    const QLocale german(QLocale::German, QLocale::Germany);
    const auto accent = [past](const QString &text, const QString &color) {
        return past ? text : QString("<span style=\"color:%1; font-weight:600;\">%2</span>").arg(color, text);
    };
    const auto date = [&german, &accent](const QDateTime &value) {
        return accent(german.toString(value.date(), "dddd dd.MM.yyyy"), "#84B7EC");
    };
    const auto time = [&accent](const QDateTime &value) {
        return accent(value.time().toString("HH:mm"), "#83C795");
    };

    const QString type = entry.isReminder() ? "Erinnerung" : "Termin";
    const QString heading = past ? type : "<b>" + type + "</b>";
    const auto start = entry.startAt.toLocalTime();
    if (!entry.endAt) {
        return heading + " am " + date(start) + " um " + time(start) + " Uhr";
    }
    const auto end = entry.endAt->toLocalTime();
    if (start.date() == end.date()) {
        return heading + " am " + date(start) + " von " + time(start) + " bis " + time(end) + " Uhr";
    }
    return heading + " von " + date(start) + " um " + time(start) + " Uhr bis "
        + date(end) + " um " + time(end) + " Uhr";
}

void showTaskPlanningDialog(QWidget *parent, ApiClient &apiClient, const Task &task) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Zeitplanung – " + task.name);
    auto *layout = new QVBoxLayout(&dialog);
    auto *scroll = new QScrollArea(&dialog);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(new QLabel("Kalendereinträge werden geladen …"));
    layout->addWidget(scroll);

    QObject::connect(&apiClient, &ApiClient::calendarEntriesReceived, &dialog,
        [scroll, &dialog, &apiClient, taskId = task.id](const std::vector<CalendarEntry> &entries) {
            auto *list = new QWidget;
            auto *rows = new QVBoxLayout(list);
            rows->setContentsMargins(0, 0, 0, 0);
            rows->setSpacing(0);
            const auto now = QDateTime::currentDateTime();
            for (const CalendarEntry &entry : entries) {
                if (entry.systemTaskId != taskId) {
                    continue;
                }
                if (rows->count() > 0) {
                    auto *separator = new QFrame(list);
                    separator->setFixedHeight(1);
                    separator->setStyleSheet("background: rgba(128, 128, 128, 55);");
                    rows->addWidget(separator);
                }

                const bool past = entry.endAt.value_or(entry.startAt) <= now;
                auto *rowWidget = new QWidget(list);
                auto *row = new QHBoxLayout(rowWidget);
                row->setContentsMargins(12, 12, 12, 12);
                if (past) {
                    auto *opacity = new QGraphicsOpacityEffect(rowWidget);
                    opacity->setOpacity(0.72);
                    rowWidget->setGraphicsEffect(opacity);
                }
                auto *text = new QVBoxLayout;
                text->setSpacing(4);
                auto *heading = new QLabel(planningHeading(entry, past), rowWidget);
                heading->setTextFormat(Qt::RichText);
                heading->setWordWrap(true);
                heading->setStyleSheet(past ? "font-size: 14px; color: #B5B5B5;" : "font-size: 16px;");
                text->addWidget(heading);
                auto *title = new QLabel(entry.name, rowWidget);
                title->setTextFormat(Qt::PlainText);
                title->setWordWrap(true);
                title->setForegroundRole(QPalette::PlaceholderText);
                title->setStyleSheet("font-size: 12px;");
                text->addWidget(title);
                row->addLayout(text, 1);

                auto *actions = new QToolButton(rowWidget);
                actions->setIcon(QIcon(":/assets/icons/option.png"));
                actions->setAccessibleName("Aktionen für " + entry.name);
                actions->setAutoRaise(true);
                actions->setStyleSheet("QToolButton::menu-indicator { image: none; }");
                actions->setPopupMode(QToolButton::InstantPopup);
                auto *menu = new QMenu(actions);
                actions->setMenu(menu);
                auto *edit = menu->addAction(QIcon(":/assets/icons/edit.png"), "Bearbeiten");
                auto *remove = menu->addAction(contextMenuDeleteIcon(), "Löschen");
                edit->setEnabled(!entry.isLocked);
                remove->setEnabled(!entry.isLocked);
                QObject::connect(edit, &QAction::triggered, &dialog, [&dialog, &apiClient, entry] {
                    editCalendarEntry(&dialog, apiClient, entry);
                });
                QObject::connect(remove, &QAction::triggered, &dialog, [&apiClient, id = entry.id] {
                    apiClient.deleteCalendarEntry(id);
                });
                row->addWidget(actions, 0, Qt::AlignTop);
                rows->addWidget(rowWidget);
            }
            if (rows->count() == 0) {
                rows->addWidget(new QLabel("Keine Erinnerungen oder Termine eingeplant.", list));
            }
            rows->addStretch();
            scroll->setWidget(list);
        });

    const auto refresh = [&apiClient] { apiClient.getCalendarEntries(); };
    QObject::connect(&apiClient, &ApiClient::calendarEntryCreated, &dialog, refresh);
    QObject::connect(&apiClient, &ApiClient::calendarEntryUpdated, &dialog, refresh);
    QObject::connect(&apiClient, &ApiClient::calendarEntryDeleted, &dialog, refresh);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    buttons->button(QDialogButtonBox::Close)->setText("Schließen");
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.resize(540, 250);
    prepareFormDialog(dialog, *buttons);
    refresh();
    execDialog(dialog);
}
