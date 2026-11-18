#include "DomainView.h"
#include "DocumentEditor.h"
#include "TaskEditDialog.h"
#include "VersionHistoryDialog.h"

#include <QFrame>
#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QTimer>
#include <QTextBrowser>
#include <QTreeView>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>
#include <QStandardPaths>

#include <functional>
#include <algorithm>

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
static constexpr int TASK_COUNT_ROLE = Qt::UserRole + 13;
static constexpr int ATTACHMENT_COUNT_ROLE = Qt::UserRole + 14;
static constexpr int FLASHCARD_BACK_ROLE = Qt::UserRole + 1;
static constexpr int ATTACHMENT_STORAGE_NAME_ROLE = Qt::UserRole + 1;

class DomainNodeDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(
        QPainter *painter,
        const QStyleOptionViewItem &option,
        const QModelIndex &index
    ) const override {
        if (!index.data(Qt::UserRole).isValid()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        struct Indicator {
            QString text;
            QColor color;
            int width;
        };
        std::vector<Indicator> indicators;
        const QFontMetrics metrics(option.font);
        auto addIndicator = [&](const int role, const QString &symbol, const QColor &color) {
            const int count = index.data(role).toInt();
            if (count > 0) {
                const QString text = symbol + " " + QString::number(count);
                indicators.push_back({text, color, metrics.horizontalAdvance(text)});
            }
        };
        addIndicator(TASK_COUNT_ROLE, "☑", QColor("#E0E0E0"));
        addIndicator(PURSUIT_COUNT_ROLE, "↑", QColor("#81C784"));
        addIndicator(SKILL_COUNT_ROLE, "★", QColor("#FFD54F"));
        addIndicator(FLASHCARD_COUNT_ROLE, "▣", QColor("#64B5F6"));
        addIndicator(ATTACHMENT_COUNT_ROLE, "📎", QColor("#BDBDBD"));
        if (indicators.empty()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        constexpr int gap = 10;
        int indicatorWidth = 0;
        for (const Indicator &indicator : indicators) {
            indicatorWidth += indicator.width;
        }
        if (!indicators.empty()) {
            indicatorWidth += gap * (static_cast<int>(indicators.size()) - 1);
        }

        QStyleOptionViewItem backgroundOption(option);
        initStyleOption(&backgroundOption, index);
        backgroundOption.text.clear();
        backgroundOption.icon = {};
        QStyle *style = option.widget
            ? option.widget->style()
            : QApplication::style();
        style->drawControl(
            QStyle::CE_ItemViewItem,
            &backgroundOption,
            painter,
            option.widget
        );

        const int indicatorLeft = indicators.empty()
            ? option.rect.right() + 1
            : option.rect.right() - indicatorWidth - 5;
        QStyleOptionViewItem nameOption(option);
        nameOption.rect.setRight(
            indicators.empty() ? option.rect.right() : indicatorLeft - 7
        );
        QStyledItemDelegate::paint(painter, nameOption, index);

        painter->save();
        int x = indicatorLeft;
        for (const Indicator &indicator : indicators) {
            painter->setPen(indicator.color);
            painter->drawText(
                QRect(x, option.rect.top(), indicator.width, option.rect.height()),
                Qt::AlignLeft | Qt::AlignVCenter,
                indicator.text
            );
            x += indicator.width + gap;
        }
        painter->restore();
    }
};

static QString metadataName(const MetadataItem &item) {
    return item.displayName.value_or(item.name);
}

static QString skillText(const Metadata &metadata, const Skill &skill) {
    QString text = skill.name;
    if (skill.psychomotorLevelId.has_value()) {
        for (const MetadataItem &level : metadata.psychomotorLevels) {
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

static QJsonValue jsonValue(const std::optional<int> value) {
    return value.has_value()
        ? QJsonValue(*value)
        : QJsonValue(QJsonValue::Null);
}

static int itemCount(const QStandardItem *parent) {
    int count = parent->rowCount();
    for (int row = 0; row < parent->rowCount(); ++row) {
        count += itemCount(parent->child(row));
    }
    return count;
}

static const MetadataItem *findMetadataItem(
    const std::vector<MetadataItem> &items,
    const std::optional<int> id
) {
    for (const MetadataItem &item : items) {
        if (id.has_value() && item.id == *id) {
            return &item;
        }
    }
    return nullptr;
}

static QString highlighted(const QString &text, const QString &color) {
    return "<span style=\"color: " + color
        + "; font-weight: 600;\">" + text + "</span>";
}

static QString attachmentDirectory() {
    const QString path = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation
    ) + "/attachments";
    QDir().mkpath(path);
    return path;
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
    item->setData(node.attachmentCount, ATTACHMENT_COUNT_ROLE);
    item->setData(node.skillCount, SKILL_COUNT_ROLE);
    item->setData(node.pursuitCount, PURSUIT_COUNT_ROLE);
    item->setData(node.taskCount, TASK_COUNT_ROLE);
    setOptionalRole(item, COGNITIVE_LEVEL_ROLE, node.cognitiveLevelId);
    setOptionalRole(item, COMPLETENESS_LEVEL_ROLE, node.completenessLevelId);
    setOptionalRole(item, FLASHCARD_STATUS_ROLE, node.flashcardStatusId);
    setOptionalRole(item, VERIFICATION_STATUS_ROLE, node.verificationStatusId);
    setOptionalRole(item, VERIFICATION_LEVEL_ROLE, node.verificationLevelId);
}

static void refreshDomainNodeText(QStandardItem *item) {
    const QString name = item->data(NODE_NAME_ROLE).toString();
    item->setText(name);
    const int flashcardCount = item->data(FLASHCARD_COUNT_ROLE).toInt();
    const int attachmentCount = item->data(ATTACHMENT_COUNT_ROLE).toInt();
    const int skillCount = item->data(SKILL_COUNT_ROLE).toInt();
    const int pursuitCount = item->data(PURSUIT_COUNT_ROLE).toInt();
    const int taskCount = item->data(TASK_COUNT_ROLE).toInt();

    QStringList details;
    if (taskCount > 0) {
        details.append("Aufgaben: " + QString::number(taskCount));
    }
    if (pursuitCount > 0) {
        details.append("Bestrebungen: " + QString::number(pursuitCount));
    }
    if (skillCount > 0) {
        details.append("Skills: " + QString::number(skillCount));
    }
    if (flashcardCount > 0) {
        details.append("Flashcards: " + QString::number(flashcardCount));
    }
    if (attachmentCount > 0) {
        details.append("Anhänge: " + QString::number(attachmentCount));
    }
    item->setToolTip(
        details.isEmpty() ? name : name + "\n" + details.join("\n")
    );
}

static void showDomainNodeDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const int nodeId,
    const Metadata &metadata,
    const QString &name,
    const std::optional<int> cognitiveLevelId,
    const bool hasDocument,
    const std::optional<int> completenessLevelId,
    const std::optional<int> flashcardStatusId,
    const std::optional<int> verificationStatusId,
    const std::optional<int> verificationLevelId
) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Domain Node bearbeiten");
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    auto *nameEdit = new QLineEdit(name, &dialog);
    auto *cognitiveCombo = new QComboBox(&dialog);
    fillMetadataCombo(cognitiveCombo, metadata.cognitiveLevels);
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
            metadata.documentCompletenessLevels
        );
        fillMetadataCombo(
            flashcardStatusCombo,
            metadata.documentFlashcardStatuses
        );
        fillMetadataCombo(
            verificationStatusCombo,
            metadata.documentVerificationStatuses
        );
        fillMetadataCombo(
            verificationLevelCombo,
            metadata.documentVerificationLevels
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

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();

    const QString updatedName = nameEdit->text().trimmed();
    if (!updatedName.isEmpty() && updatedName != name) {
        apiClient.updateDomainNodeField(
            nodeId, DomainNodeField::Name, updatedName
        );
    }

    const std::optional<int> updatedCognitiveLevelId = selectedMetadata(cognitiveCombo);
    if (updatedCognitiveLevelId != cognitiveLevelId) {
        apiClient.updateDomainNodeField(
            nodeId,
            DomainNodeField::CognitiveLevelId,
            jsonValue(updatedCognitiveLevelId)
        );
    }

    if (hasDocument) {
        const std::optional<int> updatedCompletenessLevelId =
            selectedMetadata(completenessCombo);
        if (updatedCompletenessLevelId != completenessLevelId) {
            apiClient.updateDomainNodeField(
                nodeId,
                DomainNodeField::CompletenessLevelId,
                jsonValue(updatedCompletenessLevelId)
            );
        }

        const std::optional<int> updatedFlashcardStatusId =
            selectedMetadata(flashcardStatusCombo);
        if (updatedFlashcardStatusId != flashcardStatusId) {
            apiClient.updateDomainNodeField(
                nodeId,
                DomainNodeField::FlashcardStatusId,
                jsonValue(updatedFlashcardStatusId)
            );
        }

        const std::optional<int> updatedVerificationStatusId =
            selectedMetadata(verificationStatusCombo);
        if (updatedVerificationStatusId != verificationStatusId) {
            apiClient.updateDomainNodeField(
                nodeId,
                DomainNodeField::VerificationStatusId,
                jsonValue(updatedVerificationStatusId)
            );
        }

        const std::optional<int> updatedVerificationLevelId =
            selectedMetadata(verificationLevelCombo);
        if (updatedVerificationLevelId != verificationLevelId) {
            apiClient.updateDomainNodeField(
                nodeId,
                DomainNodeField::VerificationLevelId,
                jsonValue(updatedVerificationLevelId)
            );
        }
    }
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

static void showFlashcardEditDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const int nodeId,
    const Flashcard &flashcard
) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Lernkarte bearbeiten");
    dialog.resize(480, 280);

    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    auto *frontEdit = new QPlainTextEdit(flashcard.front, &dialog);
    auto *backEdit = new QPlainTextEdit(flashcard.back, &dialog);
    form->addRow("Front", frontEdit);
    form->addRow("Back", backEdit);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();

    const QString front = frontEdit->toPlainText().trimmed();
    if (!front.isEmpty() && front != flashcard.front) {
        apiClient.updateFlashcardField(
            nodeId,
            flashcard.id,
            FlashcardField::Front,
            front
        );
    }

    const QString back = backEdit->toPlainText().trimmed();
    if (!back.isEmpty() && back != flashcard.back) {
        apiClient.updateFlashcardField(
            nodeId,
            flashcard.id,
            FlashcardField::Back,
            back
        );
    }
}

static bool showSkillDialog(
    QWidget *parent,
    const Metadata &metadata,
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
    fillMetadataCombo(levelCombo, metadata.psychomotorLevels);
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

static void showSkillEditDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const Skill &skill
) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Skill bearbeiten");
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    auto *nameEdit = new QLineEdit(skill.name, &dialog);
    auto *levelCombo = new QComboBox(&dialog);
    fillMetadataCombo(levelCombo, apiClient.metadata().psychomotorLevels);
    selectMetadata(levelCombo, skill.psychomotorLevelId);
    form->addRow("Name", nameEdit);
    form->addRow("Psychomotorisches Niveau", levelCombo);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();

    const QString name = nameEdit->text().trimmed();
    if (!name.isEmpty() && name != skill.name) {
        apiClient.updateSkillField(
            skill.domainNodeId,
            skill.id,
            SkillField::Name,
            name
        );
    }

    const std::optional<int> psychomotorLevelId = selectedMetadata(levelCombo);
    if (psychomotorLevelId != skill.psychomotorLevelId) {
        apiClient.updateSkillField(
            skill.domainNodeId,
            skill.id,
            SkillField::PsychomotorLevelId,
            jsonValue(psychomotorLevelId)
        );
    }
}

static bool showPursuitDialog(
    QWidget *parent,
    const Metadata &metadata,
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
    fillMetadataCombo(cognitiveCombo, metadata.cognitiveLevels);
    fillMetadataCombo(completenessCombo, metadata.documentCompletenessLevels);
    fillMetadataCombo(flashcardStatusCombo, metadata.documentFlashcardStatuses);
    fillMetadataCombo(verificationStatusCombo, metadata.documentVerificationStatuses);
    fillMetadataCombo(verificationLevelCombo, metadata.documentVerificationLevels);
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
            fillMetadataCombo(combo, metadata.psychomotorLevels);
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

static void showProgressionDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const Progression &progression
) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Progression bearbeiten");
    auto *layout = new QFormLayout(&dialog);
    auto *nameEdit = new QLineEdit(progression.name, &dialog);
    auto *relevanceCombo = new QComboBox(&dialog);
    auto *impactCombo = new QComboBox(&dialog);
    fillMetadataCombo(relevanceCombo, apiClient.metadata().personalRelevanceLevels);
    fillMetadataCombo(impactCombo, apiClient.metadata().lastingImpacts);
    selectMetadata(relevanceCombo, progression.personalRelevanceId);
    selectMetadata(impactCombo, progression.lastingImpactId);
    layout->addRow("Name", nameEdit);
    layout->addRow("Persönliche Relevanz", relevanceCombo);
    layout->addRow("Lasting Impact", impactCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);
    dialog.exec();

    const QString name = nameEdit->text().trimmed();
    if (!name.isEmpty() && name != progression.name) {
        apiClient.updateProgressionField(
            progression.id,
            ProgressionField::Name,
            name
        );
    }

    const std::optional<int> personalRelevanceId = selectedMetadata(relevanceCombo);
    if (personalRelevanceId != progression.personalRelevanceId) {
        apiClient.updateProgressionField(
            progression.id,
            ProgressionField::PersonalRelevanceId,
            jsonValue(personalRelevanceId)
        );
    }

    const std::optional<int> lastingImpactId = selectedMetadata(impactCombo);
    if (lastingImpactId != progression.lastingImpactId) {
        apiClient.updateProgressionField(
            progression.id,
            ProgressionField::LastingImpactId,
            jsonValue(lastingImpactId)
        );
    }
}

DomainView::DomainView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
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
        QWidget#editorHeader {
            background: rgba(255, 255, 255, 0.025);
        }
        QWidget#documentMetadataBar {
            background: rgba(255, 255, 255, 0.025);
            border-top: 1px solid rgba(255, 255, 255, 0.10);
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
    progressionWidget->setFixedHeight(40);
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
    auto *toggleProgressionButton = new QPushButton("Öffnen", progressionHeader);
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
    progressionContent->hide();
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
            [this, toggleProgressionButton, expanded = false]() mutable {
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
    treeView->setMinimumWidth(280);
    treeView->setMaximumWidth(600);
    treeView->setUniformRowHeights(true);
    treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    model = new QStandardItemModel(treeView);
    treeView->setModel(model);
    treeView->setItemDelegate(new DomainNodeDelegate(treeView));

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
        QTextBrowser#documentPreview {
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
    documentPreview = new QTextBrowser(documentWidget);
    documentPreview->setObjectName("documentPreview");
    documentPreview->document()->setDocumentMargin(16);
    documentStack = new QStackedWidget(documentWidget);
    documentStack->addWidget(documentEditor);
    documentStack->addWidget(documentPreview);
    toggleDocumentPreviewButton = new QPushButton("Vorschau", documentWidget);
    toggleDocumentPreviewButton->setObjectName("compactButton");
    saveStatus = new SaveStatus(documentWidget);
    autosaveTimer = new QTimer(this);
    autosaveTimer->setInterval(1000);
    autosaveTimer->setSingleShot(true);
    headerLayout->addWidget(documentTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(toggleDocumentPreviewButton);
    headerLayout->addWidget(saveStatus);
    documentLayout->addWidget(header);
    documentLayout->addWidget(documentStack, 1);

    connect(toggleDocumentPreviewButton, &QPushButton::clicked, this, [this] {
        const bool showPreview = documentStack->currentWidget() == documentEditor;
        if (showPreview) {
            documentPreview->setMarkdown(documentEditor->toPlainText());
        }
        documentStack->setCurrentWidget(
            showPreview ? static_cast<QWidget *>(documentPreview)
                        : static_cast<QWidget *>(documentEditor)
        );
        toggleDocumentPreviewButton->setText(
            showPreview ? "Bearbeiten" : "Vorschau"
        );
    });

    documentMetadataWidget = new QWidget(editorWidget);
    documentMetadataWidget->setObjectName("documentMetadataBar");
    auto *documentMetadataLayout = new QVBoxLayout(documentMetadataWidget);
    documentMetadataLayout->setContentsMargins(12, 6, 12, 6);
    documentMetadataLayout->setSpacing(2);
    completenessLabel = new QLabel(documentMetadataWidget);
    verificationLabel = new QLabel(documentMetadataWidget);
    for (QLabel *label : {completenessLabel, verificationLabel}) {
        label->setTextFormat(Qt::RichText);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        label->setStyleSheet("color: #B8B8B8; font-size: 11px;");
    }
    documentMetadataLayout->addWidget(completenessLabel);
    documentMetadataLayout->addWidget(verificationLabel);

    editorLayout->addWidget(documentWidget);
    editorLayout->addStretch();
    editorLayout->addWidget(documentMetadataWidget);
    editorWidget->hide();

    skillWidget = new QWidget(splitter);
    skillWidget->setObjectName("skillPanel");
    skillWidget->setFixedWidth(300);
    skillWidget->setStyleSheet(R"(
        QWidget#skillPanel, QWidget#sidebarContent,
        QWidget#taskPanel, QWidget#pursuitPanel, QWidget#skillsPanel, QWidget#flashcardsPanel,
        QWidget#attachmentsPanel,
        QScrollArea#sidebarScrollArea {
            background: palette(base);
        }
        QWidget#sidebarList {
            border: 1px solid rgba(255, 255, 255, 0.12);
            border-radius: 0;
        }
        QPushButton#taskSectionButton,
        QPushButton#pursuitSectionButton,
        QPushButton#skillSectionButton,
        QPushButton#attachmentSectionButton {
            padding: 7px 8px;
            border: none;
            border-radius: 0;
            text-align: left;
            font-weight: bold;
        }
        QPushButton#taskSectionButton {
            background: rgba(224, 224, 224, 0.12);
        }
        QPushButton#pursuitSectionButton {
            background: rgba(76, 175, 80, 0.16);
            border-top: 1px solid rgba(255, 255, 255, 0.12);
        }
        QPushButton#skillSectionButton {
            background: rgba(255, 193, 7, 0.16);
            border-top: 1px solid rgba(255, 255, 255, 0.12);
        }
        QPushButton#attachmentSectionButton {
            background: rgba(189, 189, 189, 0.16);
            border-top: 1px solid rgba(255, 255, 255, 0.12);
        }
        QWidget#flashcardSectionHeader {
            background: rgba(33, 150, 243, 0.16);
            border-top: 1px solid rgba(255, 255, 255, 0.12);
        }
        QPushButton#flashcardSectionButton {
            padding: 7px 8px;
            border: none;
            border-radius: 0;
            background: transparent;
            text-align: left;
            font-weight: bold;
        }
        QLabel#flashcardSectionStatus {
            background: transparent;
            padding-right: 8px;
            font-size: 10px;
        }
        QPushButton#taskSectionButton:hover {
            background: rgba(224, 224, 224, 0.18);
        }
        QPushButton#pursuitSectionButton:hover {
            background: rgba(76, 175, 80, 0.24);
        }
        QPushButton#skillSectionButton:hover {
            background: rgba(255, 193, 7, 0.24);
        }
        QPushButton#attachmentSectionButton:hover {
            background: rgba(189, 189, 189, 0.24);
        }
        QPushButton#flashcardSectionButton:hover {
            background: rgba(33, 150, 243, 0.08);
        }
        QTreeView#pursuitTree, QTreeView#skillTree {
            background: palette(base);
            border: none;
        }
        QListView#taskList, QListView#flashcardList, QListView#attachmentList {
            background: palette(base);
            border: none;
            outline: none;
        }
        QListView#taskList::item, QListView#flashcardList::item,
        QListView#attachmentList::item {
            border: none;
            padding: 2px 4px;
        }
        QListView#flashcardList::item:hover {
            background: rgba(33, 150, 243, 0.10);
        }
        QListView#flashcardList::item:selected {
            background: rgba(33, 150, 243, 0.18);
        }
        QListView#attachmentList::item:hover {
            background: rgba(255, 255, 255, 0.08);
        }
        QListView#attachmentList::item:selected {
            background: rgba(255, 255, 255, 0.14);
        }
    )");
    auto *skillLayout = new QVBoxLayout(skillWidget);
    skillLayout->setContentsMargins(0, 0, 0, 0);
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
    sidebarContentLayout->setSpacing(0);

    auto *sidebarList = new QWidget(sidebarContent);
    sidebarList->setObjectName("sidebarList");
    sidebarList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *sidebarListLayout = new QVBoxLayout(sidebarList);
    sidebarListLayout->setContentsMargins(1, 1, 1, 1);
    sidebarListLayout->setSpacing(0);

    auto *taskSectionButton = new QPushButton("▾  Aufgaben", sidebarList);
    taskSectionButton->setObjectName("taskSectionButton");
    taskSectionButton->setCheckable(true);
    taskSectionButton->setChecked(true);

    auto *taskPanel = new QWidget(sidebarList);
    taskPanel->setObjectName("taskPanel");
    taskPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *taskPanelLayout = new QVBoxLayout(taskPanel);
    taskPanelLayout->setContentsMargins(8, 6, 8, 6);
    taskPanelLayout->setSpacing(4);

    noTasksLabel = new QLabel("Noch keine Aufgaben angelegt.", taskPanel);
    noTasksLabel->setWordWrap(true);
    noTasksLabel->setStyleSheet("color: palette(mid);");
    taskPanelLayout->addWidget(noTasksLabel);

    taskListView = new QListView(taskPanel);
    taskListView->setObjectName("taskList");
    taskListView->setFrameShape(QFrame::NoFrame);
    taskListView->setTextElideMode(Qt::ElideRight);
    taskListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    taskListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    taskListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    taskListView->setContextMenuPolicy(Qt::CustomContextMenu);
    taskModel = new QStandardItemModel(taskListView);
    taskListView->setModel(taskModel);
    taskPanelLayout->addWidget(taskListView);

    auto *pursuitPanel = new QWidget(sidebarList);
    pursuitPanel->setObjectName("pursuitPanel");
    pursuitPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *pursuitLayout = new QVBoxLayout(pursuitPanel);
    pursuitLayout->setContentsMargins(8, 6, 8, 6);
    pursuitLayout->setSpacing(4);

    auto *pursuitSectionButton = new QPushButton("▾  Bestrebungen", sidebarList);
    pursuitSectionButton->setObjectName("pursuitSectionButton");
    pursuitSectionButton->setCheckable(true);
    pursuitSectionButton->setChecked(true);

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
    pursuitModel = new QStandardItemModel(pursuitTreeView);
    pursuitTreeView->setModel(pursuitModel);
    pursuitLayout->addWidget(pursuitTreeView);

    auto *skillPanel = new QWidget(sidebarList);
    skillPanel->setObjectName("skillsPanel");
    skillPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *skillPanelLayout = new QVBoxLayout(skillPanel);
    skillPanelLayout->setContentsMargins(8, 6, 8, 6);
    skillPanelLayout->setSpacing(4);

    auto *skillSectionButton = new QPushButton("▾  Skillansicht", sidebarList);
    skillSectionButton->setObjectName("skillSectionButton");
    skillSectionButton->setCheckable(true);
    skillSectionButton->setChecked(true);

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

    auto *flashcardPanel = new QWidget(sidebarList);
    flashcardPanel->setObjectName("flashcardsPanel");
    flashcardPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *flashcardPanelLayout = new QVBoxLayout(flashcardPanel);
    flashcardPanelLayout->setContentsMargins(8, 6, 8, 6);
    flashcardPanelLayout->setSpacing(4);

    auto *flashcardSectionHeader = new QWidget(sidebarList);
    flashcardSectionHeader->setObjectName("flashcardSectionHeader");
    auto *flashcardSectionHeaderLayout = new QHBoxLayout(flashcardSectionHeader);
    flashcardSectionHeaderLayout->setContentsMargins(0, 0, 0, 0);
    flashcardSectionHeaderLayout->setSpacing(4);
    auto *flashcardSectionButton = new QPushButton(
        "▾  Flashcards", flashcardSectionHeader
    );
    flashcardSectionButton->setObjectName("flashcardSectionButton");
    flashcardSectionButton->setCheckable(true);
    flashcardSectionButton->setChecked(true);
    flashcardSectionStatusLabel = new QLabel(flashcardSectionHeader);
    flashcardSectionStatusLabel->setObjectName("flashcardSectionStatus");
    flashcardSectionStatusLabel->hide();
    flashcardSectionHeaderLayout->addWidget(flashcardSectionButton, 1);
    flashcardSectionHeaderLayout->addWidget(flashcardSectionStatusLabel);

    noFlashcardsLabel = new QLabel("Noch keine Flashcards angelegt.", flashcardPanel);
    noFlashcardsLabel->setWordWrap(true);
    noFlashcardsLabel->setStyleSheet("color: palette(mid);");
    flashcardPanelLayout->addWidget(noFlashcardsLabel);

    flashcardListView = new QListView(flashcardPanel);
    flashcardListView->setObjectName("flashcardList");
    flashcardListView->setFrameShape(QFrame::NoFrame);
    flashcardListView->setWordWrap(false);
    flashcardListView->setTextElideMode(Qt::ElideRight);
    flashcardListView->setSpacing(0);
    flashcardListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    flashcardListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    flashcardListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    flashcardListView->setContextMenuPolicy(Qt::CustomContextMenu);
    flashcardModel = new QStandardItemModel(flashcardListView);
    flashcardListView->setModel(flashcardModel);
    flashcardPanelLayout->addWidget(flashcardListView);

    auto *attachmentSectionButton = new QPushButton("▾  Anhänge", sidebarList);
    attachmentSectionButton->setObjectName("attachmentSectionButton");
    attachmentSectionButton->setCheckable(true);
    attachmentSectionButton->setChecked(true);

    auto *attachmentPanel = new QWidget(sidebarList);
    attachmentPanel->setObjectName("attachmentsPanel");
    attachmentPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *attachmentPanelLayout = new QVBoxLayout(attachmentPanel);
    attachmentPanelLayout->setContentsMargins(8, 6, 8, 6);
    attachmentPanelLayout->setSpacing(4);

    noAttachmentsLabel = new QLabel("Noch keine Anhänge angelegt.", attachmentPanel);
    noAttachmentsLabel->setWordWrap(true);
    noAttachmentsLabel->setStyleSheet("color: palette(mid);");
    attachmentPanelLayout->addWidget(noAttachmentsLabel);

    attachmentListView = new QListView(attachmentPanel);
    attachmentListView->setObjectName("attachmentList");
    attachmentListView->setFrameShape(QFrame::NoFrame);
    attachmentListView->setTextElideMode(Qt::ElideRight);
    attachmentListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    attachmentListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    attachmentListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    attachmentListView->setContextMenuPolicy(Qt::CustomContextMenu);
    attachmentModel = new QStandardItemModel(attachmentListView);
    attachmentListView->setModel(attachmentModel);
    attachmentPanelLayout->addWidget(attachmentListView);

    sidebarListLayout->addWidget(taskSectionButton);
    sidebarListLayout->addWidget(taskPanel);
    sidebarListLayout->addWidget(pursuitSectionButton);
    sidebarListLayout->addWidget(pursuitPanel);
    sidebarListLayout->addWidget(skillSectionButton);
    sidebarListLayout->addWidget(skillPanel);
    sidebarListLayout->addWidget(flashcardSectionHeader);
    sidebarListLayout->addWidget(flashcardPanel);
    sidebarListLayout->addWidget(attachmentSectionButton);
    sidebarListLayout->addWidget(attachmentPanel);
    sidebarContentLayout->addWidget(sidebarList);
    sidebarContentLayout->addStretch();
    sidebarScrollArea->setWidget(sidebarContent);
    skillLayout->addWidget(sidebarScrollArea);
    skillWidget->hide();

    connect(taskSectionButton, &QPushButton::toggled, this,
            [taskSectionButton, taskPanel](const bool expanded) {
                taskSectionButton->setText(
                    expanded ? "▾  Aufgaben" : "▸  Aufgaben"
                );
                taskPanel->setVisible(expanded);
            });
    connect(pursuitSectionButton, &QPushButton::toggled, this,
            [pursuitSectionButton, pursuitPanel](const bool expanded) {
                pursuitSectionButton->setText(
                    expanded ? "▾  Bestrebungen" : "▸  Bestrebungen"
                );
                pursuitPanel->setVisible(expanded);
            });
    connect(skillSectionButton, &QPushButton::toggled, this,
            [skillSectionButton, skillPanel](const bool expanded) {
                skillSectionButton->setText(
                    expanded ? "▾  Skillansicht" : "▸  Skillansicht"
                );
                skillPanel->setVisible(expanded);
            });
    connect(flashcardSectionButton, &QPushButton::toggled, this,
            [flashcardSectionButton, flashcardPanel](const bool expanded) {
                flashcardSectionButton->setText(
                    expanded ? "▾  Flashcards" : "▸  Flashcards"
                );
                flashcardPanel->setVisible(expanded);
            });
    connect(attachmentSectionButton, &QPushButton::toggled, this,
            [attachmentSectionButton, attachmentPanel](const bool expanded) {
                attachmentSectionButton->setText(
                    expanded ? "▾  Anhänge" : "▸  Anhänge"
                );
                attachmentPanel->setVisible(expanded);
            });

    splitter->addWidget(treeView);
    splitter->addWidget(editorWidget);
    splitter->addWidget(skillWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({380, 1000, 300});
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
                QStandardItem *parentItem = node.parentId.has_value()
                    ? findDomainNodeItem(*node.parentId)
                    : findDomainItem(node.domainId);
                parentItem->appendRow(item);
                refreshDomainNodeText(item);
            });
    connect(&apiClient, &ApiClient::domainNodeFieldUpdated, this,
            [this](const int nodeId, const DomainNodeField field,
                   const QJsonValue &value) {
                QStandardItem *item = findDomainNodeItem(nodeId);
                switch (field) {
                    case DomainNodeField::Name:
                        item->setData(value.toString(), NODE_NAME_ROLE);
                        refreshDomainNodeText(item);
                        if (currentDocumentNodeId == nodeId) {
                            documentTitle->setText(value.toString());
                        }
                        break;
                    case DomainNodeField::CognitiveLevelId:
                        item->setData(value.isNull() ? QVariant() : value.toInt(),
                                      COGNITIVE_LEVEL_ROLE);
                        break;
                    case DomainNodeField::CompletenessLevelId:
                        item->setData(value.isNull() ? QVariant() : value.toInt(),
                                      COMPLETENESS_LEVEL_ROLE);
                        break;
                    case DomainNodeField::FlashcardStatusId:
                        item->setData(value.isNull() ? QVariant() : value.toInt(),
                                      FLASHCARD_STATUS_ROLE);
                        break;
                    case DomainNodeField::VerificationStatusId:
                        item->setData(value.isNull() ? QVariant() : value.toInt(),
                                      VERIFICATION_STATUS_ROLE);
                        break;
                    case DomainNodeField::VerificationLevelId:
                        item->setData(value.isNull() ? QVariant() : value.toInt(),
                                      VERIFICATION_LEVEL_ROLE);
                        break;
                }
                if (currentNodeId == nodeId) {
                    updateFlashcardStatus(optionalRole(item->index(), FLASHCARD_STATUS_ROLE));
                }
                if (currentDocumentNodeId == nodeId) {
                    updateDocumentStatus(
                        optionalRole(item->index(), COMPLETENESS_LEVEL_ROLE),
                        optionalRole(item->index(), VERIFICATION_STATUS_ROLE),
                        optionalRole(item->index(), VERIFICATION_LEVEL_ROLE)
                    );
                }
            });
    connect(&apiClient, &ApiClient::domainNodeDeleted, this,
            [this](const int nodeId) {
                QStandardItem *item = findDomainNodeItem(nodeId);
                QStandardItem *currentItem = currentNodeId == -1
                    ? nullptr
                    : findDomainNodeItem(currentNodeId);
                for (QStandardItem *ancestor = currentItem;
                     ancestor;
                     ancestor = ancestor->parent()) {
                    if (ancestor == item) {
                        requestDocumentChange(-1);
                        break;
                    }
                }

                item->parent()->removeRow(item->row());
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
                if (documentStack->currentWidget() == documentPreview) {
                    documentPreview->setMarkdown(document.content);
                }
                updateFlashcardStatus(document.flashcardStatusId);
                updateDocumentStatus(
                    document.completenessLevelId,
                    document.verificationStatusId,
                    document.verificationLevelId
                );
                saveStatus->setState(SaveStatus::Hidden);
                editorWidget->show();
            });
    connect(&apiClient, &ApiClient::documentLoadFailed, this,
            [this](const int nodeId) {
                if (!documentLoadPending || nodeId != pendingDocumentNodeId) {
                    return;
                }

                documentLoadPending = false;
                currentDocumentNodeId = -1;
                documentWidget->hide();
                documentMetadataWidget->hide();
                editorLayout->setStretch(0, 0);
                editorLayout->setStretch(1, 1);
            });
    connect(&apiClient, &ApiClient::documentCreated, this,
            [this](const Document &document) {
                QStandardItem *item = findDomainNodeItem(document.domainNodeId);
                item->setData(true, HAS_DOCUMENT_ROLE);
                item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
                requestDocumentChange(document.domainNodeId, true);
            });
    connect(&apiClient, &ApiClient::documentDeleted, this,
            [this](const int nodeId) {
                QStandardItem *item = findDomainNodeItem(nodeId);
                item->setData(false, HAS_DOCUMENT_ROLE);
                item->setIcon({});
                if (currentNodeId == nodeId) {
                    requestDocumentChange(nodeId, false);
                }
            });
    connect(&apiClient, &ApiClient::documentUpdated, this,
            [this](const int nodeId, const bool successful) {
                if (!saveInProgress || nodeId != savingDocumentNodeId) {
                    return;
                }

                saveInProgress = false;
                savingDocumentNodeId = -1;
                if (!successful) {
                    saveStatus->setState(SaveStatus::Error);
                    documentChangePending = false;
                    closeContinuation = {};
                    if (QStandardItem *item = findDomainNodeItem(currentNodeId)) {
                        treeView->setCurrentIndex(item->index());
                    }
                    return;
                }

                savedContent = savingContent;
                saveStatus->setState(SaveStatus::Saved);
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
    connect(flashcardListView, &QListView::clicked, this,
            [this](const QModelIndex &index) { editFlashcard(index.row()); });
    connect(taskListView, &QListView::customContextMenuRequested, this,
            [this](const QPoint &position) {
                const QModelIndex index = taskListView->indexAt(position);
                if (!index.isValid()) {
                    return;
                }

                QMenu menu(taskListView);
                QAction *editAction = menu.addAction(
                    QIcon(":/assets/icons/edit.png"),
                    "Bearbeiten"
                );
                QAction *deleteAction = menu.addAction(
                    QIcon(":/assets/icons/delete.png"),
                    "Löschen"
                );
                menu.addSeparator();
                QAction *historyAction = menu.addAction(
                    QIcon(":/assets/icons/version_history.png"),
                    "Versionshistorie"
                );
                QAction *selectedAction = menu.exec(
                    taskListView->viewport()->mapToGlobal(position)
                );
                const int taskId = index.data(Qt::UserRole).toInt();
                if (selectedAction == editAction) {
                    editTask(taskId);
                } else if (selectedAction == deleteAction) {
                    apiClient.deleteTask(taskId);
                } else if (selectedAction == historyAction) {
                    showVersionHistoryDialog(
                        taskListView,
                        apiClient,
                        taskId,
                        "Versionshistorie – " + index.data().toString()
                    );
                }
            });
    connect(flashcardListView, &QListView::customContextMenuRequested, this,
            [this](const QPoint &position) {
                const QModelIndex index = flashcardListView->indexAt(position);
                if (!index.isValid()) {
                    return;
                }

                QMenu menu(flashcardListView);
                QAction *deleteAction = menu.addAction(
                    QIcon(":/assets/icons/delete.png"),
                    "Löschen"
                );
                menu.addSeparator();
                QAction *historyAction = menu.addAction(
                    QIcon(":/assets/icons/version_history.png"),
                    "Versionshistorie"
                );
                QAction *selectedAction = menu.exec(
                    flashcardListView->viewport()->mapToGlobal(position)
                );
                if (selectedAction == deleteAction) {
                    apiClient.deleteFlashcard(
                        currentNodeId,
                        index.data(Qt::UserRole).toInt()
                    );
                } else if (selectedAction == historyAction) {
                    showVersionHistoryDialog(
                        flashcardListView,
                        apiClient,
                        index.data(Qt::UserRole).toInt(),
                        "Versionshistorie – Lernkarte"
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
                    auto *item = new QStandardItem(flashcard.front);
                    item->setData(flashcard.id, Qt::UserRole);
                    item->setData(flashcard.back, FLASHCARD_BACK_ROLE);
                    item->setIcon(QIcon(":/assets/icons/flashcards-white.png"));
                    item->setSizeHint(QSize(0, 24));
                    flashcardModel->appendRow(item);
                }
                updateFlashcardView();
            });
    connect(attachmentListView, &QListView::doubleClicked, this,
            [](const QModelIndex &index) {
                const QString path = QDir(attachmentDirectory()).filePath(
                    index.data(ATTACHMENT_STORAGE_NAME_ROLE).toString()
                );
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            });
    connect(attachmentListView, &QListView::customContextMenuRequested, this,
            [this](const QPoint &position) {
                const QModelIndex index = attachmentListView->indexAt(position);
                if (!index.isValid()) {
                    return;
                }

                QMenu menu(attachmentListView);
                QAction *deleteAction = menu.addAction(
                    QIcon(":/assets/icons/delete.png"),
                    "Löschen"
                );
                if (menu.exec(attachmentListView->viewport()->mapToGlobal(position))
                    == deleteAction) {
                    apiClient.deleteAttachment(
                        currentNodeId,
                        index.data(Qt::UserRole).toInt()
                    );
                }
            });
    connect(&apiClient, &ApiClient::nodeAttachmentsChanged, this,
            [this](const int nodeId) { apiClient.getNodeAttachments(nodeId); });
    connect(&apiClient, &ApiClient::nodeAttachmentsReceived, this,
            [this](const int nodeId, const std::vector<Attachment> &attachments) {
                updateAttachmentCount(nodeId, static_cast<int>(attachments.size()));
                if (nodeId == currentNodeId) {
                    renderAttachments(attachments);
                }
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
    connect(&apiClient, &ApiClient::nodePursuitsChanged, this,
            [this](const int nodeId) { apiClient.getNodePursuits(nodeId); });
    connect(&apiClient, &ApiClient::nodePursuitsReceived, this,
            [this](const int nodeId, const std::vector<Pursuit> &pursuits) {
                updatePursuitCount(nodeId, static_cast<int>(pursuits.size()));
                if (nodeId == currentNodeId) {
                    currentPursuits = pursuits;
                    updateCurrentTaskCount();
                    renderPursuits();
                }
            });
    connect(&apiClient, &ApiClient::taskReceived, this,
            [this](const std::vector<Task> &tasks) {
                currentTasks = tasks;
                updateCurrentTaskCount();
                renderTasks();
                renderPursuits();
            });
    connect(documentEditor, &QPlainTextEdit::textChanged, this, [this] {
        if (currentDocumentNodeId == -1) {
            return;
        }

        if (documentEditor->toPlainText() == savedContent) {
            autosaveTimer->stop();
            saveStatus->setState(SaveStatus::Saved);
        } else {
            saveStatus->setState(SaveStatus::Saving);
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
    });

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
    documentEditor->clear();
    documentPreview->clear();
    documentLoadPending = nodeId != -1 && hasDocument;
    flashcardModel->removeRows(0, flashcardModel->rowCount());
    attachmentModel->removeRows(0, attachmentModel->rowCount());
    taskModel->removeRows(0, taskModel->rowCount());
    skillModel->removeRows(0, skillModel->rowCount());
    pursuitModel->removeRows(0, pursuitModel->rowCount());
    currentSkills.clear();
    currentPursuits.clear();
    renderTasks();
    noSkillsLabel->show();
    skillTreeView->hide();
    noPursuitsLabel->show();
    pursuitTreeView->hide();
    completenessLabel->hide();
    verificationLabel->hide();
    documentMetadataWidget->hide();
    editorWidget->setVisible(nodeId != -1);
    skillWidget->setVisible(nodeId != -1);
    documentWidget->setVisible(hasDocument);
    editorLayout->setStretch(0, hasDocument ? 1 : 0);
    editorLayout->setStretch(1, hasDocument ? 0 : 1);
    updateFlashcardView();
    renderAttachments({});

    if (nodeId != -1) {
        updateFlashcardStatus(optionalRole(
            findDomainNodeItem(nodeId)->index(), FLASHCARD_STATUS_ROLE
        ));
        apiClient.getNodeFlashcards(nodeId);
        apiClient.getNodeAttachments(nodeId);
        apiClient.getNodeSkills(nodeId);
        apiClient.getNodePursuits(nodeId);
        if (hasDocument) {
            apiClient.getDocument(nodeId);
        }
    } else {
        updateFlashcardStatus(std::nullopt);
    }
}

void DomainView::startSave() {
    if (saveInProgress || currentDocumentNodeId == -1
        || documentEditor->toPlainText() == savedContent) {
        return;
    }

    saveInProgress = true;
    savingDocumentNodeId = currentDocumentNodeId;
    savingContent = documentEditor->toPlainText();
    saveStatus->setState(SaveStatus::Saving);
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
        label->setStyleSheet("color: palette(mid);");
        progressionStagesLayout->addWidget(label, 1);
        return;
    }
    if (selectedProgression->stages.empty()) {
        auto *label = new QLabel("Noch keine Progressionsstufen angelegt.");
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: palette(mid);");
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
                    QAction *assignAction = menu.addAction(
                        QIcon(":/assets/icons/add.png"),
                        "Bestrebung hinzufügen"
                    );
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
    QAction *editAction = menu.addAction(
        QIcon(":/assets/icons/edit.png"),
        "Bearbeiten"
    );
    QAction *stageAction = menu.addAction(
        QIcon(":/assets/icons/add.png"),
        "Progressionsstufe hinzufügen"
    );
    menu.addSeparator();
    QAction *historyAction = menu.addAction(
        QIcon(":/assets/icons/version_history.png"),
        "Versionshistorie"
    );
    QAction *selectedAction = menu.exec(
        progressionTreeView->viewport()->mapToGlobal(position)
    );
    if (selectedAction == editAction) {
        editProgression(progressionId);
    } else if (selectedAction == stageAction) {
        addProgressionStage(progressionId);
    } else if (selectedAction == historyAction) {
        const auto progression = std::find_if(
            progressions.begin(),
            progressions.end(),
            [progressionId](const Progression &item) {
                return item.id == progressionId;
            }
        );
        showVersionHistoryDialog(
            progressionTreeView,
            apiClient,
            progressionId,
            progression == progressions.end()
                ? "Versionshistorie"
                : "Versionshistorie – " + progression->name
        );
    }
}

void DomainView::editProgression(const int progressionId) {
    for (const Progression &progression : progressions) {
        if (progression.id != progressionId) {
            continue;
        }
        showProgressionDialog(this, apiClient, progression);
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
                if (node.hasDocument) {
                    item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
                }
                parentItem->appendRow(item);
                refreshDomainNodeText(item);
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
        QAction *createAction = menu.addAction(
            QIcon(":/assets/icons/add.png"),
            "Root-Knoten hinzufügen"
        );

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
    QAction *createAction = menu.addAction(
        QIcon(":/assets/icons/add.png"),
        "Unterknoten hinzufügen"
    );
    QAction *editAction = menu.addAction(
        QIcon(":/assets/icons/edit.png"),
        "Bearbeiten"
    );
    QAction *createDocumentAction = nullptr;
    QAction *deleteDocumentAction = nullptr;
    if (index.data(HAS_DOCUMENT_ROLE).toBool()) {
        deleteDocumentAction = menu.addAction(
            QIcon(":/assets/icons/delete.png"),
            "Dokument löschen"
        );
    } else {
        createDocumentAction = menu.addAction(
            QIcon(":/assets/icons/add.png"),
            "Dokument erstellen"
        );
    }
    menu.addSeparator();
    QAction *createTaskAction = menu.addAction(
        QIcon(":/assets/icons/add.png"),
        "Aufgabe hinzufügen"
    );
    QAction *createPursuitAction = menu.addAction(
        QIcon(":/assets/icons/add.png"),
        "Bestrebung hinzufügen"
    );
    QAction *createSkillAction = menu.addAction(
        QIcon(":/assets/icons/add.png"),
        "Skill hinzufügen"
    );
    QAction *createFlashcardAction = menu.addAction(
        QIcon(":/assets/icons/add.png"),
        "Lernkarte hinzufügen"
    );
    QAction *createAttachmentAction = menu.addAction(
        QIcon(":/assets/icons/add.png"),
        "Anhang hinzufügen"
    );
    menu.addSeparator();
    QAction *historyAction = menu.addAction(
        QIcon(":/assets/icons/version_history.png"),
        "Versionshistorie"
    );
    menu.addSeparator();
    QAction *deleteAction = menu.addAction(
        QIcon(":/assets/icons/delete.png"),
        "Löschen"
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
        showDomainNodeDialog(
            treeView,
            apiClient,
            nodeId,
            apiClient.metadata(),
            index.data(NODE_NAME_ROLE).toString(),
            optionalRole(index, COGNITIVE_LEVEL_ROLE),
            index.data(HAS_DOCUMENT_ROLE).toBool(),
            optionalRole(index, COMPLETENESS_LEVEL_ROLE),
            optionalRole(index, FLASHCARD_STATUS_ROLE),
            optionalRole(index, VERIFICATION_STATUS_ROLE),
            optionalRole(index, VERIFICATION_LEVEL_ROLE)
        );
    } else if (createDocumentAction && selectedAction == createDocumentAction) {
        apiClient.createDocument(nodeId);
    } else if (deleteDocumentAction && selectedAction == deleteDocumentAction
               && QMessageBox::question(
                   treeView,
                   "Dokument löschen",
                   "Dokument dieses Domain Nodes löschen?"
               ) == QMessageBox::Yes) {
        saveBeforeClose([this, nodeId] { apiClient.deleteDocument(nodeId); });
    } else if (selectedAction == createTaskAction) {
        createTask(nodeId);
    } else if (selectedAction == createPursuitAction) {
        createPursuit(nodeId);
    } else if (selectedAction == createSkillAction) {
        createSkill(nodeId);
    } else if (selectedAction == createFlashcardAction) {
        createFlashcard(nodeId);
    } else if (selectedAction == createAttachmentAction) {
        addAttachments(nodeId);
    } else if (selectedAction == historyAction) {
        showVersionHistoryDialog(
            treeView,
            apiClient,
            nodeId,
            "Versionshistorie – " + index.data(NODE_NAME_ROLE).toString()
        );
    } else if (selectedAction == deleteAction
               && QMessageBox::question(
                   treeView,
                   "Bereich löschen",
                   "Bereich einschließlich aller Unterbereiche löschen?"
               ) == QMessageBox::Yes) {
        saveBeforeClose([this, nodeId] { apiClient.deleteDomainNode(nodeId); });
    }
}

void DomainView::createFlashcard(const int nodeId) {
    QString front;
    QString back;
    if (showFlashcardDialog(this, "Lernkarte erstellen", front, back)) {
        apiClient.createFlashcard(nodeId, front, back);
    }
}

void DomainView::addAttachments(const int nodeId) {
    const QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Anhänge auswählen"
    );
    const QString directory = attachmentDirectory();
    for (const QString &file : files) {
        const QFileInfo source(file);
        const QString storageName = QUuid::createUuid().toString(QUuid::WithoutBraces)
            + "_" + source.fileName();
        if (QFile::copy(file, QDir(directory).filePath(storageName))) {
            apiClient.createAttachment(
                nodeId,
                source.fileName(),
                storageName,
                source.size()
            );
        }
    }
}

void DomainView::renderAttachments(const std::vector<Attachment> &attachments) {
    attachmentModel->clear();
    QFileIconProvider iconProvider;
    const QDir directory(attachmentDirectory());
    for (const Attachment &attachment : attachments) {
        auto *item = new QStandardItem(attachment.originalName);
        item->setData(attachment.id, Qt::UserRole);
        item->setData(attachment.storageName, ATTACHMENT_STORAGE_NAME_ROLE);
        item->setIcon(iconProvider.icon(QFileInfo(
            directory.filePath(attachment.storageName)
        )));
        item->setSizeHint(QSize(0, 24));
        attachmentModel->appendRow(item);
    }

    noAttachmentsLabel->setVisible(attachments.empty());
    attachmentListView->setVisible(!attachments.empty());
    attachmentListView->setFixedHeight(
        static_cast<int>(attachments.size()) * 28
    );
}

void DomainView::editFlashcard(const int row) {
    QStandardItem *frontItem = flashcardModel->item(row, 0);
    showFlashcardEditDialog(this, apiClient, currentNodeId, {
        .id = frontItem->data(Qt::UserRole).toInt(),
        .front = frontItem->text(),
        .back = frontItem->data(FLASHCARD_BACK_ROLE).toString()
    });
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

            auto *item = new QStandardItem(skillText(apiClient.metadata(), skill));
            item->setData(skill.id, Qt::UserRole);
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
    QAction *createAction = menu.addAction(
        QIcon(":/assets/icons/add.png"),
        "Unter-Skill hinzufügen"
    );
    QAction *editAction = menu.addAction(
        QIcon(":/assets/icons/edit.png"),
        "Bearbeiten"
    );
    menu.addSeparator();
    QAction *historyAction = menu.addAction(
        QIcon(":/assets/icons/version_history.png"),
        "Versionshistorie"
    );
    QAction *selectedAction = menu.exec(
        skillTreeView->viewport()->mapToGlobal(position)
    );

    if (selectedAction == createAction) {
        createSkill(currentNodeId, index.data(Qt::UserRole).toInt());
    } else if (selectedAction == editAction) {
        editSkill(index);
    } else if (selectedAction == historyAction) {
        const int skillId = index.data(Qt::UserRole).toInt();
        const auto skill = std::find_if(
            currentSkills.begin(),
            currentSkills.end(),
            [skillId](const Skill &item) { return item.id == skillId; }
        );
        showVersionHistoryDialog(
            skillTreeView,
            apiClient,
            skillId,
            skill == currentSkills.end()
                ? "Versionshistorie"
                : "Versionshistorie – " + skill->name
        );
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
            apiClient.metadata(),
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

    const int skillId = index.data(Qt::UserRole).toInt();
    const auto skill = std::find_if(
        currentSkills.begin(),
        currentSkills.end(),
        [skillId](const Skill &item) { return item.id == skillId; }
    );
    if (skill == currentSkills.end()) {
        return;
    }

    showSkillEditDialog(this, apiClient, *skill);
}

void DomainView::renderTasks() {
    taskModel->clear();

    for (const Task &task : currentTasks) {
        if (task.domainNodeId != currentNodeId) {
            continue;
        }

        auto *item = new QStandardItem(task.name);
        item->setData(task.id, Qt::UserRole);
        item->setIcon(QIcon(":/assets/icons/task-white.png"));
        item->setSizeHint(QSize(0, 24));
        taskModel->appendRow(item);
    }

    const int count = taskModel->rowCount();
    noTasksLabel->setVisible(count == 0);
    taskListView->setVisible(count > 0);
    taskListView->setFixedHeight(count * 28);
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
                        apiClient.metadata().cognitiveLevels,
                        *requirement.cognitiveLevelId
                    );
                    break;
                case 2: break;
                case 3:
                    text = "Vollständigkeit: " + metadataText(
                        apiClient.metadata().documentCompletenessLevels,
                        *requirement.completenessLevelId
                    );
                    break;
                case 4:
                    text = "Lernkarten: " + metadataText(
                        apiClient.metadata().documentFlashcardStatuses,
                        *requirement.flashcardStatusId
                    );
                    break;
                case 5:
                    text = "Verifizierung: " + metadataText(
                        apiClient.metadata().documentVerificationStatuses,
                        *requirement.verificationStatusId
                    );
                    break;
                case 6:
                    text = "Verifizierungsniveau: " + metadataText(
                        apiClient.metadata().documentVerificationLevels,
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
                    apiClient.metadata().psychomotorLevels,
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

void DomainView::createTask(const int nodeId) {
    QDialog dialog(this);
    dialog.setWindowTitle("Aufgabe erstellen");
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(&dialog);
    layout->addRow("Name", nameEdit);

    auto *priorityEdit = new QComboBox(&dialog);
    for (const MetadataItem &priority : apiClient.metadata().systemTaskPriorities) {
        priorityEdit->addItem(priority.displayName.value_or(priority.name), priority.id);
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
        .groupId = std::nullopt,
        .priorityId = priorityEdit->currentData().toInt(),
        .domainNodeId = nodeId
    });
}

void DomainView::editTask(const int taskId) {
    const auto task = std::find_if(
        currentTasks.begin(), currentTasks.end(),
        [taskId](const Task &item) { return item.id == taskId; }
    );
    if (task == currentTasks.end()) {
        return;
    }

    showTaskEditDialog(this, apiClient, *task);
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
        if (showPursuitDialog(
            this, apiClient.metadata(), pursuit, currentSkills, requirements
        )) {
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

void DomainView::updateCurrentTaskCount() {
    if (currentNodeId == -1) {
        return;
    }

    const int count = static_cast<int>(std::count_if(
        currentTasks.begin(),
        currentTasks.end(),
        [this](const Task &task) { return task.domainNodeId == currentNodeId; }
    ));
    QStandardItem *item = findDomainNodeItem(currentNodeId);
    item->setData(count, TASK_COUNT_ROLE);
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

void DomainView::updateAttachmentCount(const int nodeId, const int count) {
    QStandardItem *item = findDomainNodeItem(nodeId);
    item->setData(count, ATTACHMENT_COUNT_ROLE);
    refreshDomainNodeText(item);
}

void DomainView::updateFlashcardStatus(const std::optional<int> statusId) {
    const MetadataItem *status = findMetadataItem(
        apiClient.metadata().documentFlashcardStatuses, statusId
    );
    if (!status) {
        flashcardSectionStatusLabel->hide();
        return;
    }

    QString color = "#9E9E9E";
    if (status->name == "partially_completed") {
        color = "#FBC02D";
    } else if (status->name == "completed") {
        color = "#66BB6A";
    }

    flashcardSectionStatusLabel->setText(metadataName(*status));
    flashcardSectionStatusLabel->setStyleSheet("color: " + color + ";");
    flashcardSectionStatusLabel->show();
}

void DomainView::updateDocumentStatus(
    const std::optional<int> completenessId,
    const std::optional<int> verificationStatusId,
    const std::optional<int> verificationLevelId
) {
    const MetadataItem *completeness = findMetadataItem(
        apiClient.metadata().documentCompletenessLevels, completenessId
    );
    if (completeness) {
        QString phrase;
        QString color;
        if (completeness->name == "none") {
            phrase = "noch nicht ausgearbeitet";
            color = "#9E9E9E";
        } else if (completeness->name == "partial") {
            phrase = "teilweise vollständig";
            color = "#FBC02D";
        } else if (completeness->name == "substantial") {
            phrase = "weitgehend vollständig";
            color = "#9CCC65";
        } else {
            phrase = "vollständig";
            color = "#66BB6A";
        }
        completenessLabel->setText(
            "Das Dokument ist " + highlighted(phrase, color) + "."
        );
        completenessLabel->show();
    } else {
        completenessLabel->hide();
    }

    const MetadataItem *verificationStatus = findMetadataItem(
        apiClient.metadata().documentVerificationStatuses,
        verificationStatusId
    );
    if (verificationStatus) {
        QString statusPhrase;
        QString statusColor;
        if (verificationStatus->name == "unverified") {
            statusPhrase = "nicht verifiziert";
            statusColor = "#9E9E9E";
        } else if (verificationStatus->name == "partially_verified") {
            statusPhrase = "teilweise verifiziert";
            statusColor = "#FBC02D";
        } else {
            statusPhrase = "verifiziert";
            statusColor = "#66BB6A";
        }

        QString text = "Das Dokument ist "
            + highlighted(statusPhrase, statusColor);
        const MetadataItem *verificationLevel = findMetadataItem(
            apiClient.metadata().documentVerificationLevels,
            verificationLevelId
        );
        if (verificationLevel) {
            QString levelPhrase;
            QString levelColor;
            if (verificationLevel->name == "self_reviewed") {
                levelPhrase = "selbst überprüft";
                levelColor = "#B8B8B8";
            } else if (verificationLevel->name == "cross_checked") {
                levelPhrase = "mit Quellen abgeglichen";
                levelColor = "#64B5F6";
            } else {
                levelPhrase = "validiert";
                levelColor = "#66BB6A";
            }
            text += " und wurde " + highlighted(levelPhrase, levelColor);
        }
        verificationLabel->setText(text + ".");
        verificationLabel->show();
    } else {
        verificationLabel->hide();
    }

    documentMetadataWidget->setVisible(
        !completenessLabel->isHidden() || !verificationLabel->isHidden()
    );
}

void DomainView::updateFlashcardView() {
    const int count = flashcardModel->rowCount();
    noFlashcardsLabel->setVisible(count == 0);
    flashcardListView->setVisible(count > 0);
    flashcardListView->setFixedHeight(count * 28);
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
