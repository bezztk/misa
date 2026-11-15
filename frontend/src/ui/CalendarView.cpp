#include "CalendarView.h"

#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

CalendarView::CalendarView(QWidget *parent) : QWidget(parent) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *content = new QWidget(this);
    content->setMaximumWidth(1000);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(24, 20, 24, 20);
    contentLayout->setSpacing(14);

    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel("Kalender", content);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    title->setFont(titleFont);

    auto *createButton = new QPushButton("Neuer Eintrag", content);
    connect(createButton, &QPushButton::clicked, this, [this] {
        openCreateDialog();
    });

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(createButton);
    contentLayout->addLayout(headerLayout);

    tree = new QTreeWidget(content);
    tree->setColumnCount(3);
    tree->setHeaderLabels({"Zeit", "Name", "Art"});
    tree->setRootIsDecorated(true);
    tree->setAlternatingRowColors(true);
    tree->setUniformRowHeights(true);
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    contentLayout->addWidget(tree, 1);

    layout->addStretch();
    layout->addWidget(content, 1);
    layout->addStretch();

    connect(&apiClient, &ApiClient::calendarEntriesReceived, this,
            [this](const std::vector<CalendarEntry> &receivedEntries) {
                entries = receivedEntries;
                renderEntries();
            });
    connect(&apiClient, &ApiClient::calendarEntryCreated, this, [this] {
        apiClient.getCalendarEntries();
    });

    apiClient.getCalendarEntries();
}

void CalendarView::renderEntries() {
    tree->clear();

    std::sort(entries.begin(), entries.end(), [](const CalendarEntry &left,
                                                  const CalendarEntry &right) {
        return left.startAt < right.startAt;
    });

    QDate currentDate;
    QTreeWidgetItem *dateItem = nullptr;
    const QLocale locale(QLocale::German, QLocale::Germany);

    for (const CalendarEntry &entry : entries) {
        const QDateTime localStart = entry.startAt.toLocalTime();
        if (localStart.date() != currentDate) {
            currentDate = localStart.date();
            dateItem = new QTreeWidgetItem(tree);
            dateItem->setText(0, locale.toString(currentDate, "dddd, d. MMMM yyyy"));
            QFont dateFont = dateItem->font(0);
            dateFont.setBold(true);
            dateItem->setFont(0, dateFont);
            dateItem->setFirstColumnSpanned(true);
        }

        auto *item = new QTreeWidgetItem(dateItem);
        if (entry.isReminder()) {
            item->setText(0, localStart.toString("HH:mm"));
            item->setText(2, "Erinnerung");
        } else {
            item->setText(
                0,
                localStart.toString("HH:mm") + " – "
                    + entry.endAt->toLocalTime().toString("HH:mm")
            );
            item->setText(2, "Termin");
        }
        item->setText(1, entry.name);

        const QColor color = entry.isReminder() ? QColor("#FFB74D") : QColor("#64B5F6");
        for (int column = 0; column < tree->columnCount(); ++column) {
            item->setForeground(column, color);
        }
    }

    tree->expandAll();
}

void CalendarView::openCreateDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Kalendereintrag erstellen");

    auto *layout = new QFormLayout(&dialog);

    auto *typeEdit = new QComboBox(&dialog);
    typeEdit->addItem("Erinnerung", true);
    typeEdit->addItem("Termin", false);
    layout->addRow("Art", typeEdit);

    auto *nameEdit = new QLineEdit(&dialog);
    layout->addRow("Name", nameEdit);

    const QDateTime initialStart = QDateTime::currentDateTime();
    auto *startEdit = new QDateTimeEdit(initialStart, &dialog);
    startEdit->setCalendarPopup(true);
    startEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    layout->addRow("Start", startEdit);

    auto *endLabel = new QLabel("Ende", &dialog);
    auto *endEdit = new QDateTimeEdit(initialStart.addSecs(3600), &dialog);
    endEdit->setCalendarPopup(true);
    endEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    endEdit->setMinimumDateTime(initialStart.addSecs(60));
    layout->addRow(endLabel, endEdit);

    const auto updateType = [typeEdit, endLabel, endEdit] {
        const bool isReminder = typeEdit->currentData().toBool();
        endLabel->setVisible(!isReminder);
        endEdit->setVisible(!isReminder);
    };
    connect(typeEdit, &QComboBox::currentIndexChanged, &dialog, updateType);
    connect(startEdit, &QDateTimeEdit::dateTimeChanged, &dialog,
            [endEdit](const QDateTime &start) {
                endEdit->setMinimumDateTime(start.addSecs(60));
            });
    updateType();

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return;
    }

    const bool isReminder = typeEdit->currentData().toBool();
    CreateCalendarEntry entry{
        .name = nameEdit->text().trimmed(),
        .startAt = startEdit->dateTime(),
        .isReminder = isReminder
    };
    if (!isReminder) {
        entry.endAt = endEdit->dateTime();
    }

    apiClient.createCalendarEntry(entry);
}
