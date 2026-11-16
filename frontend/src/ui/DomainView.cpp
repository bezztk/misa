#include "DomainView.h"
#include "../models/AppState.h"

#include <QFrame>
#include <QComboBox>
#include <QCheckBox>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSplitter>
#include <QSplitterHandle>
#include <QStandardItemModel>
#include <QStyle>
#include <QSpinBox>
#include <QTableView>
#include <QTextBlock>
#include <QTimer>
#include <QTimeEdit>
#include <QTimeZone>
#include <QTreeView>
#include <QVBoxLayout>

#include <functional>

static constexpr int DOMAIN_ID_ROLE = Qt::UserRole + 1;
static constexpr int HAS_DOCUMENT_ROLE = Qt::UserRole + 2;
static constexpr int FLASHCARD_COUNT_ROLE = Qt::UserRole + 3;
static constexpr int COGNITIVE_LEVEL_ROLE = Qt::UserRole + 4;
static constexpr int COMPLETENESS_LEVEL_ROLE = Qt::UserRole + 5;
static constexpr int FLASHCARD_STATUS_ROLE = Qt::UserRole + 6;
static constexpr int VERIFICATION_STATUS_ROLE = Qt::UserRole + 7;
static constexpr int VERIFICATION_LEVEL_ROLE = Qt::UserRole + 8;
static constexpr int NODE_NAME_ROLE = Qt::UserRole + 9;
static constexpr int SKILL_COUNT_ROLE = Qt::UserRole + 10;
static constexpr int PURSUIT_COUNT_ROLE = Qt::UserRole + 11;
static constexpr int HAS_ROUTINE_ROLE = Qt::UserRole + 12;
static constexpr int SKILL_NAME_ROLE = Qt::UserRole + 1;
static constexpr int PSYCHOMOTOR_LEVEL_ROLE = Qt::UserRole + 2;

static bool parseNumberList(const QString &text, const int minimum, const int maximum,
                            QString &normalized) {
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

static bool showRoutineDialog(
    QWidget *parent,
    const QString &title,
    SaveRoutine &routine,
    const std::optional<Routine> existing = std::nullopt
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
    QObject::connect(countEnabled, &QCheckBox::toggled, &dialog, [untilEnabled](const bool checked) {
        if (checked) untilEnabled->setChecked(false);
    });
    QObject::connect(untilEnabled, &QCheckBox::toggled, &dialog, [countEnabled](const bool checked) {
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
        QString startLine;
        if (fixedTime) {
            startLine = "DTSTART;TZID=Europe/Berlin:"
                + startDateEdit->date().toString("yyyyMMdd") + 'T'
                + timeEdit->time().toString("HHmmss");
        } else {
            startLine = "DTSTART;VALUE=DATE:"
                + startDateEdit->date().toString("yyyyMMdd");
        }

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

static QString metadataName(const MetadataItem &item) {
    return item.display_name.value_or(item.name);
}

static QString skillText(const Skill &skill) {
    QString text = skill.name;
    if (skill.psychomotorLevelId.has_value()) {
        for (const MetadataItem &level : AppState::metadata.psychomotor_levels) {
            if (level.id == *skill.psychomotorLevelId) {
                text += "  ·  " + metadataName(level);
                break;
            }
        }
    }
    return text;
}

static QString metadataText(const std::vector<MetadataItem> &items, const int id) {
    for (const MetadataItem &item : items) {
        if (item.id == id) {
            return metadataName(item);
        }
    }
    return "#" + QString::number(id);
}

static void fillMetadataCombo(QComboBox *combo, const std::vector<MetadataItem> &items) {
    combo->clear();
    combo->addItem("—", QVariant());
    for (const MetadataItem &item : items) {
        combo->addItem(metadataName(item), item.id);
    }
}

static void selectMetadata(QComboBox *combo, const std::optional<int> id) {
    combo->setCurrentIndex(id.has_value() ? combo->findData(*id) : 0);
}

static std::optional<int> selectedMetadata(const QComboBox *combo) {
    return combo->currentData().isValid()
        ? std::optional<int>(combo->currentData().toInt())
        : std::nullopt;
}

static int itemCount(const QStandardItem *parent) {
    int count = parent->rowCount();
    for (int row = 0; row < parent->rowCount(); ++row) {
        count += itemCount(parent->child(row));
    }
    return count;
}

static void showMetadata(
    QLabel *label,
    const QString &title,
    const std::vector<MetadataItem> &items,
    const std::optional<int> id
) {
    QString value = "—";
    for (const MetadataItem &item : items) {
        if (id.has_value() && item.id == *id) {
            value = metadataName(item);
            break;
        }
    }
    label->setText(title + ": " + value);
}

static void setOptionalRole(
    QStandardItem *item,
    const int role,
    const std::optional<int> value
) {
    item->setData(value.has_value() ? QVariant(*value) : QVariant(), role);
}

static std::optional<int> optionalRole(const QModelIndex &index, const int role) {
    return index.data(role).isValid()
        ? std::optional<int>(index.data(role).toInt())
        : std::nullopt;
}

static void setDomainNodeRoles(QStandardItem *item, const DomainNode &node) {
    item->setData(node.name, NODE_NAME_ROLE);
    item->setData(node.flashcardCount, FLASHCARD_COUNT_ROLE);
    item->setData(node.skillCount, SKILL_COUNT_ROLE);
    item->setData(node.pursuitCount, PURSUIT_COUNT_ROLE);
    item->setData(node.hasRoutine, HAS_ROUTINE_ROLE);
    setOptionalRole(item, COGNITIVE_LEVEL_ROLE, node.cognitiveLevelId);
    setOptionalRole(item, COMPLETENESS_LEVEL_ROLE, node.completenessLevelId);
    setOptionalRole(item, FLASHCARD_STATUS_ROLE, node.flashcardStatusId);
    setOptionalRole(item, VERIFICATION_STATUS_ROLE, node.verificationStatusId);
    setOptionalRole(item, VERIFICATION_LEVEL_ROLE, node.verificationLevelId);
}

static void refreshDomainNodeText(QStandardItem *item) {
    QString text = item->data(NODE_NAME_ROLE).toString();
    const QVariant cognitiveLevel = item->data(COGNITIVE_LEVEL_ROLE);
    item->setToolTip({});
    if (cognitiveLevel.isValid()) {
        text += "  C" + QString::number(cognitiveLevel.toInt());
        for (const MetadataItem &level : AppState::metadata.cognitive_levels) {
            if (level.id == cognitiveLevel.toInt()) {
                item->setToolTip("Kognitives Niveau: " + metadataName(level));
                break;
            }
        }
    }
    const int flashcardCount = item->data(FLASHCARD_COUNT_ROLE).toInt();
    if (flashcardCount > 0) {
        text += QString::fromUtf8("  ▣ ") + QString::number(flashcardCount);
    }
    const int skillCount = item->data(SKILL_COUNT_ROLE).toInt();
    if (skillCount > 0) {
        text += QString::fromUtf8("  ★ ") + QString::number(skillCount);
    }
    const int pursuitCount = item->data(PURSUIT_COUNT_ROLE).toInt();
    if (pursuitCount > 0) {
        text += QString::fromUtf8("  ↑ ") + QString::number(pursuitCount);
    }
    item->setText(text);
}

static bool showDomainNodeDialog(
    QWidget *parent,
    QString &name,
    std::optional<int> &cognitiveLevelId,
    const bool hasDocument,
    std::optional<int> &completenessLevelId,
    std::optional<int> &flashcardStatusId,
    std::optional<int> &verificationStatusId,
    std::optional<int> &verificationLevelId
) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Domain Node bearbeiten");
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    auto *nameEdit = new QLineEdit(name, &dialog);
    auto *cognitiveCombo = new QComboBox(&dialog);
    fillMetadataCombo(cognitiveCombo, AppState::metadata.cognitive_levels);
    selectMetadata(cognitiveCombo, cognitiveLevelId);
    form->addRow("Name", nameEdit);
    form->addRow("Kognitives Niveau", cognitiveCombo);
    QComboBox *completenessCombo = nullptr;
    QComboBox *flashcardStatusCombo = nullptr;
    QComboBox *verificationStatusCombo = nullptr;
    QComboBox *verificationLevelCombo = nullptr;
    if (hasDocument) {
        completenessCombo = new QComboBox(&dialog);
        flashcardStatusCombo = new QComboBox(&dialog);
        verificationStatusCombo = new QComboBox(&dialog);
        verificationLevelCombo = new QComboBox(&dialog);
        fillMetadataCombo(
            completenessCombo,
            AppState::metadata.document_completeness_levels
        );
        fillMetadataCombo(
            flashcardStatusCombo,
            AppState::metadata.document_flashcard_statuses
        );
        fillMetadataCombo(
            verificationStatusCombo,
            AppState::metadata.document_verification_statuses
        );
        fillMetadataCombo(
            verificationLevelCombo,
            AppState::metadata.document_verification_levels
        );
        selectMetadata(completenessCombo, completenessLevelId);
        selectMetadata(flashcardStatusCombo, flashcardStatusId);
        selectMetadata(verificationStatusCombo, verificationStatusId);
        selectMetadata(verificationLevelCombo, verificationLevelId);
        form->addRow("Vollständigkeit", completenessCombo);
        form->addRow("Lernkartenstatus", flashcardStatusCombo);
        form->addRow("Verifizierungsstatus", verificationStatusCombo);
        form->addRow("Verifizierungsniveau", verificationLevelCombo);
    }
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return false;
    }

    name = nameEdit->text().trimmed();
    cognitiveLevelId = selectedMetadata(cognitiveCombo);
    if (hasDocument) {
        completenessLevelId = selectedMetadata(completenessCombo);
        flashcardStatusId = selectedMetadata(flashcardStatusCombo);
        verificationStatusId = selectedMetadata(verificationStatusCombo);
        verificationLevelId = selectedMetadata(verificationLevelCombo);
    }
    return true;
}

static bool showFlashcardDialog(
    QWidget *parent,
    const QString &title,
    QString &front,
    QString &back
) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.resize(480, 280);

    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    auto *frontEdit = new QPlainTextEdit(front, &dialog);
    auto *backEdit = new QPlainTextEdit(back, &dialog);
    form->addRow("Front", frontEdit);
    form->addRow("Back", backEdit);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        &dialog
    );
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    front = frontEdit->toPlainText().trimmed();
    back = backEdit->toPlainText().trimmed();
    return !front.isEmpty() && !back.isEmpty();
}

static bool showSkillDialog(
    QWidget *parent,
    const QString &title,
    QString &name,
    std::optional<int> &psychomotorLevelId
) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    auto *nameEdit = new QLineEdit(name, &dialog);
    auto *levelCombo = new QComboBox(&dialog);
    fillMetadataCombo(levelCombo, AppState::metadata.psychomotor_levels);
    selectMetadata(levelCombo, psychomotorLevelId);
    form->addRow("Name", nameEdit);
    form->addRow("Psychomotorisches Niveau", levelCombo);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return false;
    }

    name = nameEdit->text().trimmed();
    psychomotorLevelId = selectedMetadata(levelCombo);
    return true;
}

static bool showPursuitDialog(
    QWidget *parent,
    const Pursuit &pursuit,
    const std::vector<Skill> &skills,
    UpdatePursuitRequirements &result
) {
    for (const PursuitRequirement &requirement : pursuit.requirements) {
        switch (requirement.requirementTypeId) {
            case 1: result.cognitiveLevelId = requirement.cognitiveLevelId; break;
            case 3: result.completenessLevelId = requirement.completenessLevelId; break;
            case 4: result.flashcardStatusId = requirement.flashcardStatusId; break;
            case 5: result.verificationStatusId = requirement.verificationStatusId; break;
            case 6: result.verificationLevelId = requirement.verificationLevelId; break;
            default: break;
        }
    }

    QDialog dialog(parent);
    dialog.setWindowTitle(pursuit.name);
    dialog.resize(520, 600);
    auto *layout = new QVBoxLayout(&dialog);

    auto *nodeTitle = new QLabel("Domain- und Dokumentanforderungen", &dialog);
    QFont sectionFont = nodeTitle->font();
    sectionFont.setBold(true);
    nodeTitle->setFont(sectionFont);
    layout->addWidget(nodeTitle);
    auto *nodeForm = new QFormLayout();
    auto *cognitiveCombo = new QComboBox(&dialog);
    auto *completenessCombo = new QComboBox(&dialog);
    auto *flashcardStatusCombo = new QComboBox(&dialog);
    auto *verificationStatusCombo = new QComboBox(&dialog);
    auto *verificationLevelCombo = new QComboBox(&dialog);
    fillMetadataCombo(cognitiveCombo, AppState::metadata.cognitive_levels);
    fillMetadataCombo(completenessCombo, AppState::metadata.document_completeness_levels);
    fillMetadataCombo(flashcardStatusCombo, AppState::metadata.document_flashcard_statuses);
    fillMetadataCombo(verificationStatusCombo, AppState::metadata.document_verification_statuses);
    fillMetadataCombo(verificationLevelCombo, AppState::metadata.document_verification_levels);
    selectMetadata(cognitiveCombo, result.cognitiveLevelId);
    selectMetadata(completenessCombo, result.completenessLevelId);
    selectMetadata(flashcardStatusCombo, result.flashcardStatusId);
    selectMetadata(verificationStatusCombo, result.verificationStatusId);
    selectMetadata(verificationLevelCombo, result.verificationLevelId);
    nodeForm->addRow("Kognitives Niveau", cognitiveCombo);
    nodeForm->addRow("Vollständigkeit", completenessCombo);
    nodeForm->addRow("Lernkartenstatus", flashcardStatusCombo);
    nodeForm->addRow("Verifizierungsstatus", verificationStatusCombo);
    nodeForm->addRow("Verifizierungsniveau", verificationLevelCombo);
    layout->addLayout(nodeForm);

    auto *separator = new QFrame(&dialog);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator);
    auto *skillTitle = new QLabel("Skill-Anforderungen", &dialog);
    skillTitle->setFont(sectionFont);
    layout->addWidget(skillTitle);

    auto *skillArea = new QScrollArea(&dialog);
    skillArea->setWidgetResizable(true);
    skillArea->setFrameShape(QFrame::NoFrame);
    auto *skillContent = new QWidget(skillArea);
    auto *skillForm = new QFormLayout(skillContent);
    std::vector<std::pair<int, QComboBox *>> skillCombos;
    std::function<void(std::optional<int>, int)> addSkillRows;
    addSkillRows = [&](const std::optional<int> parentId, const int depth) {
        for (const Skill &skill : skills) {
            if (skill.parentSkillId != parentId) {
                continue;
            }

            auto *combo = new QComboBox(skillContent);
            fillMetadataCombo(combo, AppState::metadata.psychomotor_levels);
            std::optional<int> selectedLevel;
            for (const PursuitRequirement &requirement : pursuit.requirements) {
                if (requirement.requirementTypeId == 2
                    && requirement.skillId == std::optional<int>(skill.id)) {
                    selectedLevel = requirement.psychomotorLevelId;
                    break;
                }
            }
            selectMetadata(combo, selectedLevel);
            auto *skillLabel = new QLabel(skill.name, skillContent);
            skillLabel->setContentsMargins(depth * 16, 0, 0, 0);
            skillForm->addRow(skillLabel, combo);
            skillCombos.emplace_back(skill.id, combo);
            addSkillRows(skill.id, depth + 1);
        }
    };
    addSkillRows(std::nullopt, 0);
    if (skills.empty()) {
        skillForm->addRow(new QLabel("Noch keine Skills angelegt.", skillContent));
    }
    skillArea->setWidget(skillContent);
    layout->addWidget(skillArea, 1);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    result.cognitiveLevelId = selectedMetadata(cognitiveCombo);
    result.completenessLevelId = selectedMetadata(completenessCombo);
    result.flashcardStatusId = selectedMetadata(flashcardStatusCombo);
    result.verificationStatusId = selectedMetadata(verificationStatusCombo);
    result.verificationLevelId = selectedMetadata(verificationLevelCombo);
    result.skills.clear();
    for (const auto &[skillId, combo] : skillCombos) {
        if (const std::optional<int> levelId = selectedMetadata(combo)) {
            result.skills.push_back({
                .skillId = skillId,
                .psychomotorLevelId = *levelId
            });
        }
    }
    return true;
}

static bool showProgressionDialog(
    QWidget *parent,
    QString &name,
    std::optional<int> &personalRelevanceId,
    std::optional<int> &lastingImpactId
) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Progression bearbeiten");
    auto *layout = new QFormLayout(&dialog);
    auto *nameEdit = new QLineEdit(name, &dialog);
    auto *relevanceCombo = new QComboBox(&dialog);
    auto *impactCombo = new QComboBox(&dialog);
    fillMetadataCombo(relevanceCombo, AppState::metadata.personal_relevance_levels);
    fillMetadataCombo(impactCombo, AppState::metadata.lasting_impacts);
    selectMetadata(relevanceCombo, personalRelevanceId);
    selectMetadata(impactCombo, lastingImpactId);
    layout->addRow("Name", nameEdit);
    layout->addRow("Persönliche Relevanz", relevanceCombo);
    layout->addRow("Lasting Impact", impactCombo);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return false;
    }
    name = nameEdit->text().trimmed();
    personalRelevanceId = selectedMetadata(relevanceCombo);
    lastingImpactId = selectedMetadata(impactCombo);
    return true;
}

class LineNumberArea;

class SaveStatus : public QWidget {
public:
    enum State { Hidden, Saving, Saved, Error };

    explicit SaveStatus(QWidget *parent = nullptr) : QWidget(parent) {
        setFixedSize(24, 24);
        connect(&animationTimer, &QTimer::timeout, this, [this] {
            angle = (angle + 30) % 360;
            update();
        });
    }

    void setState(const State newState) {
        state = newState;
        if (state == Saving) {
            animationTimer.start(80);
            setToolTip("Wird gespeichert");
        } else {
            animationTimer.stop();
            setToolTip(state == Saved ? "Gespeichert" : state == Error ? "Speichern fehlgeschlagen" : "");
        }
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        if (state == Saving) {
            QPen pen(QColor(70, 145, 230), 2);
            pen.setCapStyle(Qt::RoundCap);
            painter.setPen(pen);
            painter.drawArc(rect().adjusted(4, 4, -4, -4), (90 - angle) * 16, 270 * 16);
        } else if (state == Saved) {
            painter.setPen(QPen(QColor(90, 140, 195), 1.5));
            painter.drawLine(5, 12, 10, 17);
            painter.drawLine(10, 17, 19, 7);
        } else if (state == Error) {
            painter.setPen(QColor(220, 80, 60));
            painter.setFont(QFont(painter.font().family(), 16));
            painter.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("\xE2\x9A\xA0"));
        }
    }

private:
    State state = Hidden;
    QTimer animationTimer;
    int angle = 0;
};

class DocumentEditor : public QPlainTextEdit {
public:
    explicit DocumentEditor(QWidget *parent = nullptr);
    int lineNumberAreaWidth() const;
    void paintLineNumbers(QPaintEvent *event);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);

    QWidget *lineNumberArea;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(DocumentEditor *editor)
        : QWidget(editor), editor(editor) {}

    QSize sizeHint() const override {
        return {editor->lineNumberAreaWidth(), 0};
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        editor->paintLineNumbers(event);
    }

private:
    DocumentEditor *editor;
};

DocumentEditor::DocumentEditor(QWidget *parent)
    : QPlainTextEdit(parent), lineNumberArea(new LineNumberArea(this)) {
    connect(this, &QPlainTextEdit::blockCountChanged, this,
            [this] { updateLineNumberAreaWidth(); });
    connect(this, &QPlainTextEdit::updateRequest, this,
            [this](const QRect &rect, const int dy) {
                updateLineNumberArea(rect, dy);
            });
    updateLineNumberAreaWidth();
}

int DocumentEditor::lineNumberAreaWidth() const {
    int digits = 1;
    for (int count = qMax(1, blockCount()); count >= 10; count /= 10) {
        ++digits;
    }

    return 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void DocumentEditor::updateLineNumberAreaWidth() {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void DocumentEditor::updateLineNumberArea(const QRect &rect, const int dy) {
    if (dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth();
    }
}

void DocumentEditor::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);
    const QRect contents = contentsRect();
    lineNumberArea->setGeometry(
        QRect(contents.left(), contents.top(), lineNumberAreaWidth(), contents.height())
    );
}

void DocumentEditor::paintLineNumbers(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), palette().color(QPalette::Base));
    painter.setPen(palette().color(QPalette::Light));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.drawText(
                0,
                top,
                lineNumberArea->width() - 4,
                fontMetrics().height(),
                Qt::AlignRight,
                QString::number(blockNumber + 1)
            );
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

DomainView::DomainView(QWidget *parent) : QWidget(parent) {
    setObjectName("domainView");
    setStyleSheet(R"(
        QWidget#domainView {
            background: palette(base);
        }
        QWidget#progressionPanel {
            background: palette(base);
            border-bottom: 1px solid rgba(255, 255, 255, 0.25);
        }
        QWidget#progressionHeader {
            border-bottom: 1px solid rgba(255, 255, 255, 0.12);
        }
        QFrame#stageCard {
            background: rgba(255, 255, 255, 0.025);
            border: none;
            border-right: 1px solid rgba(255, 255, 255, 0.14);
        }
        QLabel#stageTitle {
            padding-bottom: 5px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.08);
        }
        QTreeView#domainTree, QTreeView#pursuitTree, QTreeView#skillTree {
            background: palette(base);
            border: none;
            outline: none;
        }
        QTreeView::item {
            padding: 2px 4px;
        }
        QTreeView::item:hover {
            background: rgba(255, 255, 255, 0.05);
        }
        QTreeView::item:selected {
            background: rgba(100, 181, 246, 0.18);
        }
        QWidget#editorHeader, QWidget#flashcardHeader {
            background: rgba(255, 255, 255, 0.025);
        }
        QWidget#documentMetadataBar {
            background: rgba(255, 255, 255, 0.025);
            border-top: 1px solid rgba(255, 255, 255, 0.10);
        }
        QTableView#flashcardTable {
            background: palette(base);
            alternate-background-color: rgba(255, 255, 255, 0.025);
            border: none;
            gridline-color: transparent;
            selection-background-color: rgba(100, 181, 246, 0.18);
        }
        QTableView#flashcardTable QHeaderView::section {
            background: rgba(255, 255, 255, 0.05);
            border: none;
            border-bottom: 1px solid rgba(255, 255, 255, 0.12);
            padding: 5px;
        }
        QPushButton#compactButton {
            padding: 4px 10px;
        }
        QPushButton#squareButton {
            padding: 0;
            border: 1px solid rgba(255, 255, 255, 0.14);
            border-radius: 4px;
            background: rgba(255, 255, 255, 0.04);
        }
        QPushButton#squareButton:hover {
            background: rgba(255, 255, 255, 0.10);
        }
    )");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    progressionWidget = new QWidget(this);
    progressionWidget->setObjectName("progressionPanel");
    progressionWidget->setFixedHeight(200);
    auto *progressionLayout = new QVBoxLayout(progressionWidget);
    progressionLayout->setContentsMargins(0, 0, 0, 0);
    progressionLayout->setSpacing(0);

    auto *progressionHeader = new QWidget(progressionWidget);
    progressionHeader->setObjectName("progressionHeader");
    auto *progressionHeaderLayout = new QHBoxLayout(progressionHeader);
    progressionHeaderLayout->setContentsMargins(10, 4, 10, 4);
    auto *progressionTitle = new QLabel("Progressionen", progressionHeader);
    QFont progressionTitleFont = progressionTitle->font();
    progressionTitleFont.setBold(true);
    progressionTitle->setFont(progressionTitleFont);
    auto *addProgressionButton = new QPushButton("Progression erstellen", progressionHeader);
    addProgressionButton->setObjectName("compactButton");
    auto *toggleProgressionButton = new QPushButton("Minimieren", progressionHeader);
    toggleProgressionButton->setObjectName("compactButton");
    progressionHeaderLayout->addWidget(progressionTitle);
    progressionHeaderLayout->addStretch();
    progressionHeaderLayout->addWidget(addProgressionButton);
    progressionHeaderLayout->addWidget(toggleProgressionButton);
    progressionLayout->addWidget(progressionHeader);

    progressionContent = new QWidget(progressionWidget);
    auto *progressionContentLayout = new QHBoxLayout(progressionContent);
    progressionContentLayout->setContentsMargins(0, 0, 0, 0);
    progressionContentLayout->setSpacing(0);

    auto *progressionListPanel = new QWidget(progressionContent);
    progressionListPanel->setFixedWidth(400);
    auto *progressionListLayout = new QVBoxLayout(progressionListPanel);
    progressionListLayout->setContentsMargins(8, 4, 8, 8);
    progressionListLayout->setSpacing(0);
    progressionTreeView = new QTreeView(progressionListPanel);
    progressionTreeView->setHeaderHidden(true);
    progressionTreeView->setFrameShape(QFrame::NoFrame);
    progressionTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    progressionTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    progressionModel = new QStandardItemModel(progressionTreeView);
    progressionTreeView->setModel(progressionModel);
    progressionListLayout->addWidget(progressionTreeView);
    progressionContentLayout->addWidget(progressionListPanel);

    auto *progressionSeparator = new QFrame(progressionContent);
    progressionSeparator->setFixedWidth(1);
    progressionSeparator->setStyleSheet(
        "background: rgba(255, 255, 255, 0.20); border: none;"
    );
    progressionContentLayout->addWidget(progressionSeparator);

    auto *stageScrollArea = new QScrollArea(progressionContent);
    stageScrollArea->setObjectName("progressionStages");
    stageScrollArea->setWidgetResizable(true);
    stageScrollArea->setFrameShape(QFrame::NoFrame);
    stageScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    auto *stageContent = new QWidget(stageScrollArea);
    stageContent->setObjectName("progressionStageContent");
    stageScrollArea->viewport()->setStyleSheet(
        "QWidget#qt_scrollarea_viewport { background: rgba(255, 255, 255, 0.012); }"
    );
    stageContent->setStyleSheet(
        "QWidget#progressionStageContent { background: rgba(255, 255, 255, 0.012); }"
    );
    progressionStagesLayout = new QHBoxLayout(stageContent);
    progressionStagesLayout->setContentsMargins(8, 4, 8, 8);
    progressionStagesLayout->setSpacing(6);
    stageScrollArea->setWidget(stageContent);
    progressionContentLayout->addWidget(stageScrollArea, 1);
    progressionLayout->addWidget(progressionContent, 1);
    layout->addWidget(progressionWidget);

    connect(addProgressionButton, &QPushButton::clicked, this, [this] {
        bool accepted;
        const QString name = QInputDialog::getText(
            this,
            "Progression hinzufügen",
            "Name",
            QLineEdit::Normal,
            {},
            &accepted
        ).trimmed();
        if (accepted && !name.isEmpty()) {
            apiClient.createProgression(name);
        }
    });
    connect(toggleProgressionButton, &QPushButton::clicked, this,
            [this, toggleProgressionButton, expanded = true]() mutable {
                expanded = !expanded;
                progressionContent->setVisible(expanded);
                progressionWidget->setFixedHeight(expanded ? 200 : 40);
                toggleProgressionButton->setText(expanded ? "Minimieren" : "Öffnen");
            });

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet(
        "QSplitter::handle { background: rgba(255, 255, 255, 0.25); }"
    );

    treeView = new QTreeView(splitter);
    treeView->setObjectName("domainTree");
    treeView->setHeaderHidden(true);
    treeView->setFrameShape(QFrame::NoFrame);
    treeView->setIndentation(16);
    treeView->setMinimumWidth(500);
    treeView->setMaximumWidth(800);
    treeView->setUniformRowHeights(true);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    model = new QStandardItemModel(treeView);
    treeView->setModel(model);

    editorWidget = new QWidget(splitter);
    editorWidget->setObjectName("documentEditorPanel");
    editorWidget->setStyleSheet(R"(
        QWidget#documentEditorPanel {
            background: palette(base);
        }
        QPlainTextEdit#documentEditor {
            background: palette(base);
            border: none;
        }
    )");
    editorLayout = new QVBoxLayout(editorWidget);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);

    documentWidget = new QWidget(editorWidget);
    auto *documentLayout = new QVBoxLayout(documentWidget);
    documentLayout->setContentsMargins(0, 0, 0, 0);
    documentLayout->setSpacing(0);

    auto *header = new QWidget(documentWidget);
    header->setObjectName("editorHeader");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(14, 8, 12, 8);
    documentTitle = new QLabel(documentWidget);
    QFont titleFont = documentTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    documentTitle->setFont(titleFont);
    documentEditor = new DocumentEditor(documentWidget);
    documentEditor->setObjectName("documentEditor");
    saveStatus = new SaveStatus(documentWidget);
    autosaveTimer = new QTimer(this);
    autosaveTimer->setInterval(1000);
    autosaveTimer->setSingleShot(true);
    headerLayout->addWidget(documentTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(saveStatus);
    documentLayout->addWidget(header);
    documentLayout->addWidget(documentEditor, 1);

    documentMetadataWidget = new QWidget(editorWidget);
    documentMetadataWidget->setObjectName("documentMetadataBar");
    auto *documentMetadataLayout = new QHBoxLayout(documentMetadataWidget);
    documentMetadataLayout->setContentsMargins(12, 6, 12, 6);
    documentMetadataLayout->setSpacing(14);
    completenessLabel = new QLabel(documentMetadataWidget);
    flashcardStatusLabel = new QLabel(documentMetadataWidget);
    verificationStatusLabel = new QLabel(documentMetadataWidget);
    verificationLevelLabel = new QLabel(documentMetadataWidget);
    for (QLabel *label : {
             completenessLabel,
             flashcardStatusLabel,
             verificationStatusLabel,
             verificationLevelLabel
         }) {
        label->setStyleSheet("color: #B8B8B8; font-size: 11px;");
    }
    documentMetadataLayout->addStretch();
    documentMetadataLayout->addWidget(completenessLabel);
    documentMetadataLayout->addWidget(flashcardStatusLabel);
    documentMetadataLayout->addWidget(verificationStatusLabel);
    documentMetadataLayout->addWidget(verificationLevelLabel);

    flashcardWidget = new QWidget(editorWidget);
    auto *flashcardLayout = new QVBoxLayout(flashcardWidget);
    flashcardLayout->setContentsMargins(0, 0, 0, 0);
    flashcardLayout->setSpacing(0);

    auto *flashcardSeparator = new QFrame(flashcardWidget);
    flashcardSeparator->setFixedHeight(1);
    flashcardSeparator->setStyleSheet(
        "background: rgba(255, 255, 255, 0.25); border: none;"
    );
    flashcardLayout->addWidget(flashcardSeparator);

    auto *flashcardHeader = new QWidget(flashcardWidget);
    flashcardHeader->setObjectName("flashcardHeader");
    flashcardHeader->setFixedHeight(36);
    auto *flashcardHeaderLayout = new QHBoxLayout(flashcardHeader);
    flashcardHeaderLayout->setContentsMargins(10, 4, 10, 4);
    flashcardTitle = new QLabel("Lernkarten (0)", flashcardHeader);
    toggleFlashcardsButton = new QPushButton("Lernkarten ausblenden", flashcardHeader);
    toggleFlashcardsButton->setObjectName("compactButton");
    flashcardHeaderLayout->addWidget(flashcardTitle);
    flashcardHeaderLayout->addStretch();
    flashcardHeaderLayout->addWidget(toggleFlashcardsButton);
    flashcardLayout->addWidget(flashcardHeader);

    flashcardTable = new QTableView(flashcardWidget);
    flashcardTable->setObjectName("flashcardTable");
    flashcardTable->setFrameShape(QFrame::NoFrame);
    flashcardTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    flashcardTable->setSelectionMode(QAbstractItemView::SingleSelection);
    flashcardTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    flashcardTable->setMinimumHeight(0);
    flashcardTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    flashcardTable->setContextMenuPolicy(Qt::CustomContextMenu);
    flashcardTable->setAlternatingRowColors(true);
    flashcardTable->setShowGrid(false);
    flashcardModel = new QStandardItemModel(0, 2, flashcardTable);
    flashcardModel->setHorizontalHeaderLabels({"Front", "Back"});
    flashcardTable->setModel(flashcardModel);
    flashcardTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(flashcardTable->horizontalHeader(), &QHeaderView::sectionResized,
            this, [this] { updateFlashcardView(); });
    flashcardLayout->addWidget(flashcardTable, 1);

    editorLayout->addWidget(documentWidget);
    editorLayout->addStretch();
    editorLayout->addWidget(documentMetadataWidget);
    editorLayout->addWidget(flashcardWidget);
    editorWidget->hide();

    skillWidget = new QWidget(splitter);
    skillWidget->setObjectName("skillPanel");
    skillWidget->setFixedWidth(300);
    skillWidget->setStyleSheet(R"(
        QWidget#skillPanel, QWidget#sidebarContent,
        QWidget#pursuitPanel, QWidget#skillsPanel,
        QScrollArea#sidebarScrollArea {
            background: palette(base);
        }
        QTreeView#pursuitTree, QTreeView#skillTree {
            background: palette(base);
            border: none;
        }
    )");
    auto *skillLayout = new QVBoxLayout(skillWidget);
    skillLayout->setContentsMargins(4, 0, 4, 0);
    skillLayout->setSpacing(0);

    auto *sidebarScrollArea = new QScrollArea(skillWidget);
    sidebarScrollArea->setObjectName("sidebarScrollArea");
    sidebarScrollArea->setWidgetResizable(true);
    sidebarScrollArea->setFrameShape(QFrame::NoFrame);
    sidebarScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebarScrollArea->viewport()->setStyleSheet(
        "QWidget#qt_scrollarea_viewport { background: palette(base); }"
    );
    auto *sidebarContent = new QWidget(sidebarScrollArea);
    sidebarContent->setObjectName("sidebarContent");
    auto *sidebarContentLayout = new QVBoxLayout(sidebarContent);
    sidebarContentLayout->setContentsMargins(0, 0, 0, 0);
    sidebarContentLayout->setSpacing(32);

    auto *pursuitPanel = new QWidget(sidebarContent);
    pursuitPanel->setObjectName("pursuitPanel");
    pursuitPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *pursuitLayout = new QVBoxLayout(pursuitPanel);
    pursuitLayout->setContentsMargins(0, 6, 0, 4);
    pursuitLayout->setSpacing(4);

    auto *pursuitHeaderLayout = new QHBoxLayout();
    pursuitHeaderLayout->setContentsMargins(0, 0, 0, 0);
    auto *pursuitTitle = new QLabel("Bestrebungen", pursuitPanel);
    QFont pursuitTitleFont = pursuitTitle->font();
    pursuitTitleFont.setBold(true);
    pursuitTitleFont.setPointSize(pursuitTitleFont.pointSize() + 2);
    pursuitTitle->setFont(pursuitTitleFont);
    pursuitHeaderLayout->addWidget(pursuitTitle);
    pursuitHeaderLayout->addStretch();
    pursuitLayout->addLayout(pursuitHeaderLayout);

    noPursuitsLabel = new QLabel("Noch keine Bestrebungen angelegt.", pursuitPanel);
    noPursuitsLabel->setWordWrap(true);
    noPursuitsLabel->setStyleSheet("color: palette(mid);");
    pursuitLayout->addWidget(noPursuitsLabel);

    pursuitTreeView = new QTreeView(pursuitPanel);
    pursuitTreeView->setObjectName("pursuitTree");
    pursuitTreeView->setHeaderHidden(true);
    pursuitTreeView->setFrameShape(QFrame::NoFrame);
    pursuitTreeView->setIndentation(14);
    pursuitTreeView->setUniformRowHeights(true);
    pursuitTreeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pursuitTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pursuitTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    pursuitModel = new QStandardItemModel(pursuitTreeView);
    pursuitTreeView->setModel(pursuitModel);
    pursuitLayout->addWidget(pursuitTreeView);

    auto *skillPanel = new QWidget(sidebarContent);
    skillPanel->setObjectName("skillsPanel");
    skillPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *skillPanelLayout = new QVBoxLayout(skillPanel);
    skillPanelLayout->setContentsMargins(0, 4, 0, 4);
    skillPanelLayout->setSpacing(4);

    auto *skillHeaderLayout = new QHBoxLayout();
    skillHeaderLayout->setContentsMargins(0, 0, 0, 0);
    auto *skillTitle = new QLabel("Skillansicht", skillPanel);
    QFont skillTitleFont = skillTitle->font();
    skillTitleFont.setBold(true);
    skillTitleFont.setPointSize(skillTitleFont.pointSize() + 2);
    skillTitle->setFont(skillTitleFont);
    skillHeaderLayout->addWidget(skillTitle);
    skillHeaderLayout->addStretch();
    skillPanelLayout->addLayout(skillHeaderLayout);

    noSkillsLabel = new QLabel("Noch keine Skills angelegt.", skillPanel);
    noSkillsLabel->setWordWrap(true);
    noSkillsLabel->setStyleSheet("color: palette(mid);");
    skillPanelLayout->addWidget(noSkillsLabel);

    skillTreeView = new QTreeView(skillPanel);
    skillTreeView->setObjectName("skillTree");
    skillTreeView->setHeaderHidden(true);
    skillTreeView->setFrameShape(QFrame::NoFrame);
    skillTreeView->setIndentation(14);
    skillTreeView->setUniformRowHeights(true);
    skillTreeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    skillTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    skillTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    skillModel = new QStandardItemModel(skillTreeView);
    skillTreeView->setModel(skillModel);
    skillPanelLayout->addWidget(skillTreeView);
    sidebarContentLayout->addWidget(pursuitPanel);
    sidebarContentLayout->addWidget(skillPanel);
    sidebarContentLayout->addStretch();
    sidebarScrollArea->setWidget(sidebarContent);
    skillLayout->addWidget(sidebarScrollArea);
    skillWidget->hide();

    splitter->addWidget(treeView);
    splitter->addWidget(editorWidget);
    splitter->addWidget(skillWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);
    splitter->setCollapsible(2, false);
    splitter->handle(2)->setEnabled(false);
    splitter->handle(2)->setCursor(Qt::ArrowCursor);
    layout->addWidget(splitter);

    connect(&apiClient, &ApiClient::domainTreeReceived, this,
            [this](const std::vector<MetadataItem> &domains,
                   const std::vector<DomainNode> &nodes) {
                renderDomainTree(domains, nodes);
            });
    connect(&apiClient, &ApiClient::domainNodeCreated, this,
            [this](const DomainNode &node) {
                auto *item = new QStandardItem();
                item->setData(node.id, Qt::UserRole);
                item->setData(node.hasDocument, HAS_DOCUMENT_ROLE);
                setDomainNodeRoles(item, node);
                refreshDomainNodeText(item);
                QStandardItem *parentItem = node.parentId.has_value()
                    ? findDomainNodeItem(*node.parentId)
                    : findDomainItem(node.domainId);
                parentItem->appendRow(item);
            });
    connect(&apiClient, &ApiClient::domainNodeUpdated, this,
            [this](const DomainNode &node) {
                QStandardItem *item = findDomainNodeItem(node.id);
                setDomainNodeRoles(item, node);
                refreshDomainNodeText(item);
                if (currentDocumentNodeId == node.id) {
                    documentTitle->setText(node.name);
                    showMetadata(completenessLabel, "Vollständigkeit",
                                 AppState::metadata.document_completeness_levels,
                                 node.completenessLevelId);
                    showMetadata(flashcardStatusLabel, "Lernkarten",
                                 AppState::metadata.document_flashcard_statuses,
                                 node.flashcardStatusId);
                    showMetadata(verificationStatusLabel, "Verifizierung",
                                 AppState::metadata.document_verification_statuses,
                                 node.verificationStatusId);
                    showMetadata(verificationLevelLabel, "Niveau",
                                 AppState::metadata.document_verification_levels,
                                 node.verificationLevelId);
                }
            });
    connect(&apiClient, &ApiClient::domainNodeRoutineReceived, this,
            [this](const int, const Routine &routine) {
                editRoutine(routine);
            });
    connect(&apiClient, &ApiClient::domainNodeRoutineSaved, this,
            [this](const int nodeId, const Routine &) {
                QStandardItem *item = findDomainNodeItem(nodeId);
                if (item) {
                    item->setData(true, HAS_ROUTINE_ROLE);
                }
            });
    connect(&apiClient, &ApiClient::documentReceived, this,
            [this](const Document &document) {
                if (!documentLoadPending || document.domainNodeId != pendingDocumentNodeId) {
                    return;
                }

                documentLoadPending = false;
                QStandardItem *item = findDomainNodeItem(document.domainNodeId);
                item->setData(true, HAS_DOCUMENT_ROLE);
                item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
                treeView->setCurrentIndex(item->index());
                currentDocumentNodeId = document.domainNodeId;
                documentTitle->setText(item->data(NODE_NAME_ROLE).toString());
                savedContent = document.content;
                documentEditor->setPlainText(document.content);
                showMetadata(completenessLabel, "Vollständigkeit",
                             AppState::metadata.document_completeness_levels,
                             document.completenessLevelId);
                showMetadata(flashcardStatusLabel, "Lernkarten",
                             AppState::metadata.document_flashcard_statuses,
                             document.flashcardStatusId);
                showMetadata(verificationStatusLabel, "Verifizierung",
                             AppState::metadata.document_verification_statuses,
                             document.verificationStatusId);
                showMetadata(verificationLevelLabel, "Niveau",
                             AppState::metadata.document_verification_levels,
                             document.verificationLevelId);
                static_cast<SaveStatus *>(saveStatus)->setState(SaveStatus::Hidden);
                editorWidget->show();
            });
    connect(&apiClient, &ApiClient::documentCreated, this,
            [this](const Document &document) {
                QStandardItem *item = findDomainNodeItem(document.domainNodeId);
                item->setData(true, HAS_DOCUMENT_ROLE);
                item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
                requestDocumentChange(document.domainNodeId, true);
            });
    connect(&apiClient, &ApiClient::documentUpdated, this,
            [this](const bool successful) {
                saveInProgress = false;
                if (!successful) {
                    static_cast<SaveStatus *>(saveStatus)->setState(SaveStatus::Error);
                    return;
                }

                savedContent = savingContent;
                static_cast<SaveStatus *>(saveStatus)->setState(SaveStatus::Saved);
                if (documentChangePending || closeContinuation || !autosaveTimer->isActive()) {
                    finishPendingActions();
                }
            });
    connect(treeView, &QTreeView::customContextMenuRequested, this,
            [this](const QPoint &position) {
                showDomainNodeContextMenu(position);
            });
    connect(treeView, &QTreeView::clicked, this, [this](const QModelIndex &index) {
        const QModelIndex nodeIndex = index.sibling(index.row(), 0);
        if (!nodeIndex.data(Qt::UserRole).isValid()) {
            requestDocumentChange(-1);
            return;
        }

        requestDocumentChange(
            nodeIndex.data(Qt::UserRole).toInt(),
            nodeIndex.data(HAS_DOCUMENT_ROLE).toBool()
        );
    });
    connect(toggleFlashcardsButton, &QPushButton::clicked, this, [this] {
        flashcardsExpanded = !flashcardsExpanded;
        updateFlashcardView();
    });
    connect(flashcardTable, &QTableView::doubleClicked, this,
            [this](const QModelIndex &index) { editFlashcard(index.row()); });
    connect(flashcardTable, &QTableView::customContextMenuRequested, this,
            [this](const QPoint &position) {
                const QModelIndex index = flashcardTable->indexAt(position);
                if (!index.isValid()) {
                    return;
                }

                QMenu menu(flashcardTable);
                QAction *deleteAction = menu.addAction("Löschen");
                if (menu.exec(flashcardTable->viewport()->mapToGlobal(position)) == deleteAction) {
                    apiClient.deleteFlashcard(
                        currentNodeId,
                        flashcardModel->item(index.row(), 0)->data(Qt::UserRole).toInt()
                    );
                }
            });
    connect(&apiClient, &ApiClient::nodeFlashcardsChanged, this,
            [this](const int nodeId) { apiClient.getNodeFlashcards(nodeId); });
    connect(&apiClient, &ApiClient::nodeFlashcardsReceived, this,
            [this](const int nodeId, const std::vector<Flashcard> &flashcards) {
                updateFlashcardCount(nodeId, static_cast<int>(flashcards.size()));
                if (nodeId != currentNodeId) {
                    return;
                }

                flashcardModel->removeRows(0, flashcardModel->rowCount());
                for (const Flashcard &flashcard : flashcards) {
                    auto *front = new QStandardItem(flashcard.front);
                    auto *back = new QStandardItem(flashcard.back);
                    front->setData(flashcard.id, Qt::UserRole);
                    flashcardModel->appendRow({front, back});
                }
                updateFlashcardView();
            });
    connect(skillTreeView, &QTreeView::customContextMenuRequested, this,
            [this](const QPoint &position) { showSkillContextMenu(position); });
    connect(skillTreeView, &QTreeView::doubleClicked, this,
            [this](const QModelIndex &index) { editSkill(index); });
    connect(&apiClient, &ApiClient::nodeSkillsChanged, this,
            [this](const int nodeId) { apiClient.getNodeSkills(nodeId); });
    connect(&apiClient, &ApiClient::nodeSkillsReceived, this,
            [this](const int nodeId, const std::vector<Skill> &skills) {
                updateSkillCount(nodeId, static_cast<int>(skills.size()));
                if (nodeId == currentNodeId) {
                    renderSkills(skills);
                }
            });
    connect(pursuitTreeView, &QTreeView::clicked, this,
            [this](const QModelIndex &index) { editPursuit(index); });
    connect(pursuitTreeView, &QTreeView::customContextMenuRequested, this,
            [this](const QPoint &position) { showPursuitContextMenu(position); });
    connect(&apiClient, &ApiClient::nodePursuitsChanged, this,
            [this](const int nodeId) { apiClient.getNodePursuits(nodeId); });
    connect(&apiClient, &ApiClient::nodePursuitsReceived, this,
            [this](const int nodeId, const std::vector<Pursuit> &pursuits) {
                updatePursuitCount(nodeId, static_cast<int>(pursuits.size()));
                if (nodeId == currentNodeId) {
                    currentPursuits = pursuits;
                    renderPursuits();
                }
            });
    connect(&apiClient, &ApiClient::taskReceived, this,
            [this](const std::vector<Task> &tasks) {
                currentTasks = tasks;
                renderPursuits();
            });
    connect(&apiClient, &ApiClient::taskCreated, this,
            [this] { apiClient.getTasks(); });
    connect(documentEditor, &QPlainTextEdit::textChanged, this, [this] {
        if (currentDocumentNodeId == -1) {
            return;
        }

        if (documentEditor->toPlainText() == savedContent) {
            autosaveTimer->stop();
            static_cast<SaveStatus *>(saveStatus)->setState(SaveStatus::Saved);
        } else {
            static_cast<SaveStatus *>(saveStatus)->setState(SaveStatus::Saving);
            autosaveTimer->start();
        }
    });
    connect(autosaveTimer, &QTimer::timeout, this, &DomainView::startSave);
    connect(progressionTreeView, &QTreeView::clicked, this,
            [this](const QModelIndex &index) {
                currentProgressionId = index.data(Qt::UserRole).toInt();
                renderProgressionStages();
            });
    connect(progressionTreeView, &QTreeView::customContextMenuRequested, this,
            [this](const QPoint &position) { showProgressionContextMenu(position); });
    connect(&apiClient, &ApiClient::progressionsReceived, this,
            [this](const std::vector<Progression> &receivedProgressions,
                   const std::vector<ProgressionPursuit> &receivedPursuits) {
                progressions = receivedProgressions;
                progressionPursuits = receivedPursuits;
                renderProgressions();
            });
    connect(&apiClient, &ApiClient::progressionsChanged, this,
            [this] { apiClient.getProgressions(); });
    connect(&apiClient, &ApiClient::availableProgressionPursuitsReceived, this,
            [this](const int stageId,
                   const std::vector<ProgressionPursuit> &pursuits) {
                showAvailablePursuits(stageId, pursuits);
            });
    connect(&apiClient, &ApiClient::metadataReceived, this, [this] {
        apiClient.getDomainTree();
        apiClient.getProgressions();
        apiClient.getTasks();
    });

    apiClient.getMetadata();
}

void DomainView::saveBeforeClose(std::function<void()> continuation) {
    closeContinuation = std::move(continuation);
    autosaveTimer->stop();
    finishPendingActions();
}

void DomainView::refreshTasks() {
    apiClient.getTasks();
}

void DomainView::requestDocumentChange(const int nodeId, const bool hasDocument) {
    const bool currentHasDocument = currentDocumentNodeId != -1 || documentLoadPending;
    if (nodeId == currentNodeId && hasDocument == currentHasDocument
        && !documentChangePending) {
        return;
    }

    pendingDocumentNodeId = nodeId;
    pendingNodeHasDocument = hasDocument;
    documentChangePending = true;
    autosaveTimer->stop();
    finishPendingActions();
}

void DomainView::continueDocumentChange() {
    const int nodeId = pendingDocumentNodeId;
    const bool hasDocument = pendingNodeHasDocument;
    documentChangePending = false;
    currentNodeId = nodeId;
    currentDocumentNodeId = -1;
    savedContent.clear();
    documentLoadPending = nodeId != -1 && hasDocument;
    flashcardModel->removeRows(0, flashcardModel->rowCount());
    skillModel->removeRows(0, skillModel->rowCount());
    pursuitModel->removeRows(0, pursuitModel->rowCount());
    currentSkills.clear();
    currentPursuits.clear();
    noSkillsLabel->show();
    skillTreeView->hide();
    noPursuitsLabel->show();
    pursuitTreeView->hide();
    editorWidget->setVisible(nodeId != -1);
    skillWidget->setVisible(nodeId != -1);
    documentWidget->setVisible(hasDocument);
    documentMetadataWidget->setVisible(hasDocument);
    editorLayout->setStretch(0, hasDocument ? 1 : 0);
    editorLayout->setStretch(1, hasDocument ? 0 : 1);
    updateFlashcardView();

    if (nodeId != -1) {
        apiClient.getNodeFlashcards(nodeId);
        apiClient.getNodeSkills(nodeId);
        apiClient.getNodePursuits(nodeId);
        if (hasDocument) {
            apiClient.getDocument(nodeId);
        }
    }
}

void DomainView::startSave() {
    if (saveInProgress || currentDocumentNodeId == -1
        || documentEditor->toPlainText() == savedContent) {
        return;
    }

    saveInProgress = true;
    savingContent = documentEditor->toPlainText();
    static_cast<SaveStatus *>(saveStatus)->setState(SaveStatus::Saving);
    apiClient.updateDocument(currentDocumentNodeId, savingContent);
}

void DomainView::finishPendingActions() {
    if (saveInProgress) {
        return;
    }

    if (currentDocumentNodeId != -1 && documentEditor->toPlainText() != savedContent) {
        startSave();
        return;
    }

    if (documentChangePending) {
        continueDocumentChange();
    }

    if (closeContinuation) {
        auto continuation = std::move(closeContinuation);
        closeContinuation = {};
        continuation();
    }
}

void DomainView::renderProgressions() {
    progressionModel->clear();
    progressionModel->setColumnCount(1);

    bool selectionExists = false;
    for (const Progression &progression : progressions) {
        QString text = progression.name;
        if (progression.personalRelevanceId.has_value()) {
            text += "  · R" + QString::number(*progression.personalRelevanceId);
        }
        if (progression.lastingImpactId.has_value()) {
            text += "  · I" + QString::number(*progression.lastingImpactId);
        }
        auto *item = new QStandardItem(text);
        item->setData(progression.id, Qt::UserRole);
        progressionModel->appendRow(item);
        if (progression.id == currentProgressionId) {
            progressionTreeView->setCurrentIndex(item->index());
            selectionExists = true;
        }
    }

    if (!selectionExists) {
        currentProgressionId = progressions.empty() ? -1 : progressions.front().id;
        if (!progressions.empty()) {
            progressionTreeView->setCurrentIndex(progressionModel->index(0, 0));
        }
    }
    renderProgressionStages();
}

void DomainView::renderProgressionStages() {
    while (QLayoutItem *item = progressionStagesLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const Progression *selectedProgression = nullptr;
    for (const Progression &progression : progressions) {
        if (progression.id == currentProgressionId) {
            selectedProgression = &progression;
            break;
        }
    }

    if (!selectedProgression) {
        auto *label = new QLabel("Noch keine Progression ausgewählt.");
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: #777777;");
        progressionStagesLayout->addWidget(label, 1);
        return;
    }
    if (selectedProgression->stages.empty()) {
        auto *label = new QLabel("Noch keine Progressionsstufen angelegt.");
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: #777777;");
        progressionStagesLayout->addWidget(label, 1);
        return;
    }

    for (const ProgressionStage &stage : selectedProgression->stages) {
        auto *card = new QFrame();
        card->setObjectName("stageCard");
        card->setMinimumWidth(180);
        card->setContextMenuPolicy(Qt::CustomContextMenu);
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 8, 10, 8);
        cardLayout->setSpacing(5);
        auto *title = new QLabel(
            QString::number(stage.stage) + ". " + stage.name,
            card
        );
        title->setObjectName("stageTitle");
        QFont titleFont = title->font();
        titleFont.setBold(true);
        title->setFont(titleFont);
        title->setWordWrap(true);
        cardLayout->addWidget(title);

        for (const ProgressionPursuit &pursuit : stage.pursuits) {
            auto *label = new QLabel("↑ " + pursuit.name, card);
            label->setWordWrap(true);
            label->setStyleSheet("color: #B8B8B8;");
            cardLayout->addWidget(label);
        }
        cardLayout->addStretch();
        connect(card, &QWidget::customContextMenuRequested, this,
                [this, card, stageId = stage.id](const QPoint &position) {
                    QMenu menu(card);
                    QAction *assignAction = menu.addAction("Bestrebung hinzufügen");
                    if (menu.exec(card->mapToGlobal(position)) == assignAction) {
                        assignPursuit(stageId);
                    }
                });
        progressionStagesLayout->addWidget(card, 1);
    }
}

void DomainView::showProgressionContextMenu(const QPoint &position) {
    const QModelIndex index = progressionTreeView->indexAt(position);
    if (!index.isValid()) {
        return;
    }
    const int progressionId = index.data(Qt::UserRole).toInt();
    QMenu menu(progressionTreeView);
    QAction *editAction = menu.addAction("Bearbeiten");
    QAction *stageAction = menu.addAction("Progressionsstufe hinzufügen");
    QAction *selectedAction = menu.exec(
        progressionTreeView->viewport()->mapToGlobal(position)
    );
    if (selectedAction == editAction) {
        editProgression(progressionId);
    } else if (selectedAction == stageAction) {
        addProgressionStage(progressionId);
    }
}

void DomainView::editProgression(const int progressionId) {
    for (const Progression &progression : progressions) {
        if (progression.id != progressionId) {
            continue;
        }
        QString name = progression.name;
        std::optional<int> relevanceId = progression.personalRelevanceId;
        std::optional<int> impactId = progression.lastingImpactId;
        if (showProgressionDialog(this, name, relevanceId, impactId)) {
            apiClient.updateProgression(progression.id, name, relevanceId, impactId);
        }
        return;
    }
}

void DomainView::addProgressionStage(const int progressionId) {
    bool accepted;
    const QString name = QInputDialog::getText(
        this,
        "Progressionsstufe hinzufügen",
        "Name",
        QLineEdit::Normal,
        {},
        &accepted
    ).trimmed();
    if (accepted && !name.isEmpty()) {
        apiClient.createProgressionStage(progressionId, name);
    }
}

void DomainView::assignPursuit(const int stageId) {
    apiClient.getAvailableProgressionPursuits(stageId);
}

void DomainView::showAvailablePursuits(
    const int stageId,
    const std::vector<ProgressionPursuit> &pursuits
) {
    QStringList names;
    std::vector<int> ids;
    for (const ProgressionPursuit &pursuit : pursuits) {
        names.push_back(pursuit.name);
        ids.push_back(pursuit.id);
    }
    if (names.empty()) {
        QMessageBox::information(
            this,
            "Bestrebung hinzufügen",
            "Es gibt keine unzugeordneten Bestrebungen."
        );
        return;
    }

    bool accepted;
    const QString name = QInputDialog::getItem(
        this,
        "Bestrebung hinzufügen",
        "Bestrebung",
        names,
        0,
        false,
        &accepted
    );
    if (accepted) {
        apiClient.assignPursuitToStage(stageId, ids[names.indexOf(name)]);
    }
}

void DomainView::renderDomainTree(
    const std::vector<MetadataItem> &domains,
    const std::vector<DomainNode> &nodes
) {
    model->clear();
    model->setColumnCount(1);

    for (const MetadataItem &domain : domains) {
        auto *domainItem = new QStandardItem(domain.name);
        QFont font = domainItem->font();
        font.setBold(true);
        domainItem->setFont(font);
        domainItem->setData(domain.id, DOMAIN_ID_ROLE);
        model->appendRow(domainItem);

        std::function<void(std::optional<int>, QStandardItem *)> addNodes;
        addNodes = [&](const std::optional<int> parentId, QStandardItem *parentItem) {
            for (const DomainNode &node : nodes) {
                if (node.domainId != domain.id || node.parentId != parentId) {
                    continue;
                }

                auto *item = new QStandardItem();
                item->setData(node.id, Qt::UserRole);
                item->setData(node.hasDocument, HAS_DOCUMENT_ROLE);
                setDomainNodeRoles(item, node);
                refreshDomainNodeText(item);
                if (node.hasDocument) {
                    item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
                }
                parentItem->appendRow(item);
                addNodes(node.id, item);
            }
        };

        addNodes(std::nullopt, domainItem);
    }

    treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    treeView->collapseAll();
}

void DomainView::showDomainNodeContextMenu(const QPoint &position) {
    QModelIndex index = treeView->indexAt(position);

    if (!index.isValid()) {
        return;
    }

    index = index.sibling(index.row(), 0);

    if (index.data(DOMAIN_ID_ROLE).isValid()) {
        QMenu menu(treeView);
        QAction *createAction = menu.addAction("Root-Knoten hinzufügen");

        if (menu.exec(treeView->viewport()->mapToGlobal(position)) == createAction) {
            bool accepted;
            const QString name = QInputDialog::getText(
                treeView,
                "Root-Knoten hinzufügen",
                "Name",
                QLineEdit::Normal,
                {},
                &accepted
            ).trimmed();

            if (accepted && !name.isEmpty()) {
                apiClient.createRootDomainNode(index.data(DOMAIN_ID_ROLE).toInt(), name);
            }
        }

        return;
    }

    QMenu menu(treeView);
    QAction *createAction = menu.addAction("Unterknoten hinzufügen");
    QAction *editAction = menu.addAction("Bearbeiten");
    QAction *createDocumentAction = nullptr;
    if (!index.data(HAS_DOCUMENT_ROLE).toBool()) {
        createDocumentAction = menu.addAction("Dokument erstellen");
    }
    menu.addSeparator();
    QAction *createFlashcardAction = menu.addAction("Lernkarte hinzufügen");
    QAction *createPursuitAction = menu.addAction("Bestrebung hinzufügen");
    QAction *createSkillAction = menu.addAction("Skill hinzufügen");
    QAction *routineAction = menu.addAction(
        index.data(HAS_ROUTINE_ROLE).toBool()
            ? "Routine bearbeiten"
            : "Routine hinzufügen"
    );
    QAction *selectedAction = menu.exec(treeView->viewport()->mapToGlobal(position));
    const int nodeId = index.data(Qt::UserRole).toInt();

    if (selectedAction == createAction) {
        bool accepted;
        const QString name = QInputDialog::getText(
            treeView,
            "Unterknoten hinzufügen",
            "Name",
            QLineEdit::Normal,
            {},
            &accepted
        ).trimmed();

        if (accepted && !name.isEmpty()) {
            apiClient.createDomainNode(nodeId, name);
        }
    } else if (selectedAction == editAction) {
        QString name = index.data(NODE_NAME_ROLE).toString();
        std::optional<int> cognitiveLevelId = optionalRole(index, COGNITIVE_LEVEL_ROLE);
        std::optional<int> completenessLevelId = optionalRole(
            index, COMPLETENESS_LEVEL_ROLE
        );
        std::optional<int> flashcardStatusId = optionalRole(index, FLASHCARD_STATUS_ROLE);
        std::optional<int> verificationStatusId = optionalRole(
            index, VERIFICATION_STATUS_ROLE
        );
        std::optional<int> verificationLevelId = optionalRole(
            index, VERIFICATION_LEVEL_ROLE
        );

        if (showDomainNodeDialog(
                treeView,
                name,
                cognitiveLevelId,
                index.data(HAS_DOCUMENT_ROLE).toBool(),
                completenessLevelId,
                flashcardStatusId,
                verificationStatusId,
                verificationLevelId
            )) {
            apiClient.updateDomainNode(
                nodeId,
                name,
                cognitiveLevelId,
                completenessLevelId,
                flashcardStatusId,
                verificationStatusId,
                verificationLevelId
            );
        }
    } else if (createDocumentAction && selectedAction == createDocumentAction) {
        apiClient.createDocument(nodeId);
    } else if (selectedAction == createFlashcardAction) {
        createFlashcard(nodeId);
    } else if (selectedAction == createPursuitAction) {
        createPursuit(nodeId);
    } else if (selectedAction == createSkillAction) {
        createSkill(nodeId);
    } else if (selectedAction == routineAction) {
        if (index.data(HAS_ROUTINE_ROLE).toBool()) {
            apiClient.getDomainNodeRoutine(nodeId);
        } else {
            createRoutine(nodeId, index.data(NODE_NAME_ROLE).toString());
        }
    }
}

void DomainView::createRoutine(const int nodeId, const QString &nodeName) {
    SaveRoutine routine{
        .name = nodeName,
        .timezone = "Europe/Berlin",
        .estimatedDuration = 30,
        .isOptional = false,
        .isActive = true
    };
    if (showRoutineDialog(this, "Routine hinzufügen", routine)) {
        apiClient.createDomainNodeRoutine(nodeId, routine);
    }
}

void DomainView::editRoutine(const Routine &existing) {
    SaveRoutine routine{
        .name = existing.name,
        .recurrenceRuleSet = existing.recurrenceRuleSet,
        .timezone = existing.timezone,
        .estimatedDuration = existing.estimatedDuration,
        .isOptional = existing.isOptional,
        .isActive = existing.isActive
    };
    if (showRoutineDialog(this, "Routine bearbeiten", routine, existing)) {
        apiClient.updateDomainNodeRoutine(existing.domainNodeId, routine);
    }
}

void DomainView::createFlashcard(const int nodeId) {
    QString front;
    QString back;
    if (showFlashcardDialog(this, "Lernkarte erstellen", front, back)) {
        apiClient.createFlashcard(nodeId, front, back);
    }
}

void DomainView::editFlashcard(const int row) {
    QStandardItem *frontItem = flashcardModel->item(row, 0);
    QStandardItem *backItem = flashcardModel->item(row, 1);
    QString front = frontItem->text();
    QString back = backItem->text();

    if (showFlashcardDialog(this, "Lernkarte bearbeiten", front, back)) {
        apiClient.updateFlashcard(
            currentNodeId,
            frontItem->data(Qt::UserRole).toInt(),
            front,
            back
        );
    }
}

void DomainView::renderSkills(const std::vector<Skill> &skills) {
    currentSkills = skills;
    skillModel->clear();
    skillModel->setColumnCount(1);

    std::function<void(std::optional<int>, QStandardItem *)> addSkills;
    addSkills = [&](const std::optional<int> parentId, QStandardItem *parentItem) {
        for (const Skill &skill : skills) {
            if (skill.parentSkillId != parentId) {
                continue;
            }

            auto *item = new QStandardItem(skillText(skill));
            item->setData(skill.id, Qt::UserRole);
            item->setData(skill.name, SKILL_NAME_ROLE);
            setOptionalRole(item, PSYCHOMOTOR_LEVEL_ROLE, skill.psychomotorLevelId);
            if (parentItem) {
                parentItem->appendRow(item);
            } else {
                skillModel->appendRow(item);
            }
            addSkills(skill.id, item);
        }
    };
    addSkills(std::nullopt, nullptr);

    noSkillsLabel->setVisible(skills.empty());
    skillTreeView->setVisible(!skills.empty());
    skillTreeView->expandAll();
    skillTreeView->setFixedHeight(
        qMax(
            40,
            itemCount(skillModel->invisibleRootItem())
                * skillTreeView->sizeHintForRow(0) + 6
        )
    );
    renderPursuits();
}

void DomainView::showSkillContextMenu(const QPoint &position) {
    const QModelIndex index = skillTreeView->indexAt(position);
    if (!index.isValid()) {
        return;
    }

    QMenu menu(skillTreeView);
    QAction *createAction = menu.addAction("Unter-Skill hinzufügen");
    QAction *editAction = menu.addAction("Bearbeiten");
    QAction *selectedAction = menu.exec(
        skillTreeView->viewport()->mapToGlobal(position)
    );

    if (selectedAction == createAction) {
        createSkill(currentNodeId, index.data(Qt::UserRole).toInt());
    } else if (selectedAction == editAction) {
        editSkill(index);
    }
}

void DomainView::createSkill(
    const int nodeId,
    const std::optional<int> parentSkillId
) {
    QString name;
    std::optional<int> psychomotorLevelId;
    if (showSkillDialog(
            this,
            parentSkillId.has_value() ? "Unter-Skill hinzufügen" : "Skill hinzufügen",
            name,
            psychomotorLevelId
        )) {
        apiClient.createSkill(
            nodeId,
            parentSkillId,
            name,
            psychomotorLevelId
        );
    }
}

void DomainView::editSkill(const QModelIndex &index) {
    if (!index.isValid() || currentNodeId == -1) {
        return;
    }

    QString name = index.data(SKILL_NAME_ROLE).toString();
    std::optional<int> psychomotorLevelId = optionalRole(
        index,
        PSYCHOMOTOR_LEVEL_ROLE
    );
    if (showSkillDialog(this, "Skill bearbeiten", name, psychomotorLevelId)) {
        apiClient.updateSkill(
            currentNodeId,
            index.data(Qt::UserRole).toInt(),
            name,
            psychomotorLevelId
        );
    }
}

void DomainView::renderPursuits() {
    pursuitModel->clear();
    pursuitModel->setColumnCount(1);

    for (const Pursuit &pursuit : currentPursuits) {
        auto *pursuitItem = new QStandardItem(pursuit.name);
        pursuitItem->setData(pursuit.id, Qt::UserRole);
        QFont font = pursuitItem->font();
        font.setBold(true);
        pursuitItem->setFont(font);

        for (const PursuitRequirement &requirement : pursuit.requirements) {
            QString text;
            switch (requirement.requirementTypeId) {
                case 1:
                    text = "Kognition: " + metadataText(
                        AppState::metadata.cognitive_levels,
                        *requirement.cognitiveLevelId
                    );
                    break;
                case 2: break;
                case 3:
                    text = "Vollständigkeit: " + metadataText(
                        AppState::metadata.document_completeness_levels,
                        *requirement.completenessLevelId
                    );
                    break;
                case 4:
                    text = "Lernkarten: " + metadataText(
                        AppState::metadata.document_flashcard_statuses,
                        *requirement.flashcardStatusId
                    );
                    break;
                case 5:
                    text = "Verifizierung: " + metadataText(
                        AppState::metadata.document_verification_statuses,
                        *requirement.verificationStatusId
                    );
                    break;
                case 6:
                    text = "Verifizierungsniveau: " + metadataText(
                        AppState::metadata.document_verification_levels,
                        *requirement.verificationLevelId
                    );
                    break;
                default:
                    break;
            }
            if (!text.isEmpty()) {
                pursuitItem->appendRow(new QStandardItem(text));
            }
        }

        std::function<QStandardItem *(const Skill &)> skillRequirementItem;
        skillRequirementItem = [&](const Skill &skill) -> QStandardItem * {
            const PursuitRequirement *skillRequirement = nullptr;
            for (const PursuitRequirement &requirement : pursuit.requirements) {
                if (requirement.requirementTypeId == 2
                    && requirement.skillId == std::optional<int>(skill.id)) {
                    skillRequirement = &requirement;
                    break;
                }
            }

            std::vector<QStandardItem *> childItems;
            for (const Skill &child : currentSkills) {
                if (child.parentSkillId == std::optional<int>(skill.id)) {
                    if (QStandardItem *childItem = skillRequirementItem(child)) {
                        childItems.push_back(childItem);
                    }
                }
            }

            if (!skillRequirement && childItems.empty()) {
                return nullptr;
            }

            QString text = skill.name;
            if (skillRequirement) {
                text += ": " + metadataText(
                    AppState::metadata.psychomotor_levels,
                    *skillRequirement->psychomotorLevelId
                );
            }
            auto *item = new QStandardItem(text);
            for (QStandardItem *childItem : childItems) {
                item->appendRow(childItem);
            }
            return item;
        };

        for (const Skill &skill : currentSkills) {
            if (!skill.parentSkillId.has_value()) {
                if (QStandardItem *skillItem = skillRequirementItem(skill)) {
                    pursuitItem->appendRow(skillItem);
                }
            }
        }

        for (const Task &task : currentTasks) {
            if (task.pursuit_id == pursuit.id) {
                auto *taskItem = new QStandardItem("Aufgabe: " + task.name);
                taskItem->setIcon(QIcon(":/assets/icons/task-white.png"));
                pursuitItem->appendRow(taskItem);
            }
        }
        pursuitModel->appendRow(pursuitItem);
    }

    noPursuitsLabel->setVisible(currentPursuits.empty());
    pursuitTreeView->setVisible(!currentPursuits.empty());
    pursuitTreeView->expandAll();
    pursuitTreeView->setFixedHeight(
        qMax(
            40,
            itemCount(pursuitModel->invisibleRootItem())
                * pursuitTreeView->sizeHintForRow(0) + 6
        )
    );
}

void DomainView::showPursuitContextMenu(const QPoint &position) {
    const QModelIndex index = pursuitTreeView->indexAt(position);
    if (!index.isValid() || !index.data(Qt::UserRole).isValid()) {
        return;
    }

    QMenu menu(pursuitTreeView);
    QAction *createTaskAction = menu.addAction("Aufgabe erstellen");
    if (menu.exec(pursuitTreeView->viewport()->mapToGlobal(position)) == createTaskAction) {
        createPursuitTask(index.data(Qt::UserRole).toInt());
    }
}

void DomainView::createPursuitTask(const int pursuitId) {
    QDialog dialog(this);
    dialog.setWindowTitle("Aufgabe erstellen");
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(&dialog);
    layout->addRow("Name", nameEdit);

    auto *groupEdit = new QComboBox(&dialog);
    for (const MetadataItem &group : AppState::metadata.systemtask_groups) {
        groupEdit->addItem(group.name, group.id);
    }
    layout->addRow("Gruppe", groupEdit);

    auto *priorityEdit = new QComboBox(&dialog);
    for (const MetadataItem &priority : AppState::metadata.systemtask_priorities) {
        priorityEdit->addItem(priority.display_name.value_or(priority.name), priority.id);
    }
    layout->addRow("Priorität", priorityEdit);

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

    apiClient.createTask(CreateTask{
        .name = nameEdit->text().trimmed(),
        .group_id = groupEdit->currentData().toInt(),
        .priority_id = priorityEdit->currentData().toInt(),
        .pursuit_id = pursuitId
    });
}

void DomainView::createPursuit(const int nodeId) {
    bool accepted;
    const QString name = QInputDialog::getText(
        this,
        "Bestrebung hinzufügen",
        "Name",
        QLineEdit::Normal,
        {},
        &accepted
    ).trimmed();
    if (accepted && !name.isEmpty()) {
        apiClient.createPursuit(nodeId, name);
    }
}

void DomainView::editPursuit(const QModelIndex &index) {
    if (!index.isValid() || !index.data(Qt::UserRole).isValid()) {
        return;
    }

    const int pursuitId = index.data(Qt::UserRole).toInt();
    for (const Pursuit &pursuit : currentPursuits) {
        if (pursuit.id != pursuitId) {
            continue;
        }

        UpdatePursuitRequirements requirements;
        if (showPursuitDialog(this, pursuit, currentSkills, requirements)) {
            apiClient.updatePursuitRequirements(
                currentNodeId,
                pursuit.id,
                requirements
            );
        }
        return;
    }
}

void DomainView::updatePursuitCount(const int nodeId, const int count) {
    QStandardItem *item = findDomainNodeItem(nodeId);
    item->setData(count, PURSUIT_COUNT_ROLE);
    refreshDomainNodeText(item);
}

void DomainView::updateSkillCount(const int nodeId, const int count) {
    QStandardItem *item = findDomainNodeItem(nodeId);
    item->setData(count, SKILL_COUNT_ROLE);
    refreshDomainNodeText(item);
}

void DomainView::updateFlashcardCount(const int nodeId, const int count) {
    QStandardItem *item = findDomainNodeItem(nodeId);
    item->setData(count, FLASHCARD_COUNT_ROLE);
    refreshDomainNodeText(item);
}

void DomainView::updateFlashcardView() {
    const int count = flashcardModel->rowCount();
    flashcardTitle->setText("Lernkarten (" + QString::number(count) + ")");
    flashcardTitle->setVisible(count > 0);
    toggleFlashcardsButton->setVisible(count > 0);
    toggleFlashcardsButton->setText(
        flashcardsExpanded ? "Lernkarten ausblenden" : "Lernkarten anzeigen"
    );

    flashcardTable->setVisible(flashcardsExpanded && count > 0);
    int panelHeight = 37;

    if (flashcardsExpanded && count > 0) {
        flashcardTable->resizeRowsToContents();
        const int maximumRowHeight = flashcardTable->fontMetrics().lineSpacing() * 4 + 8;
        int height = flashcardTable->horizontalHeader()->height()
            + flashcardTable->frameWidth() * 2;
        for (int row = 0; row < count; ++row) {
            flashcardTable->setRowHeight(
                row,
                qMin(flashcardTable->rowHeight(row), maximumRowHeight)
            );
            height += flashcardTable->rowHeight(row);
        }

        const int maximumPanelHeight = qMax(37, editorWidget->height() / 2);
        panelHeight = qMin(37 + height, maximumPanelHeight);
        flashcardTable->setFixedHeight(panelHeight - 37);
    }

    flashcardWidget->setFixedHeight(panelHeight);
}

void DomainView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateFlashcardView();
}

QStandardItem *DomainView::findDomainItem(const int domainId) {
    const QModelIndexList matches = model->match(
        model->index(0, 0),
        DOMAIN_ID_ROLE,
        domainId,
        1,
        Qt::MatchExactly
    );

    return model->itemFromIndex(matches.first());
}

QStandardItem *DomainView::findDomainNodeItem(const int nodeId) {
    const QModelIndexList matches = model->match(
        model->index(0, 0),
        Qt::UserRole,
        nodeId,
        1,
        Qt::MatchExactly | Qt::MatchRecursive
    );

    return model->itemFromIndex(matches.first());
}
