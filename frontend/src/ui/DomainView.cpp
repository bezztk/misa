#include "DomainView.h"
#include "DialogUtils.h"
#include "CommentUi.h"
#include "ContextMenuStyle.h"
#include "DocumentEditor.h"
#include "TaskEditDialog.h"
#include "VersionHistoryDialog.h"

#include <QAbstractItemModel>
#include <QAbstractScrollArea>
#include <QButtonGroup>
#include <QFrame>
#include <QApplication>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSet>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QTimer>
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
static constexpr int KNOWLEDGE_LEVEL_ROLE = Qt::UserRole + 4;
static constexpr int COMPLETENESS_LEVEL_ROLE = Qt::UserRole + 5;
static constexpr int FLASHCARD_STATUS_ROLE = Qt::UserRole + 6;
static constexpr int VERIFICATION_STATUS_ROLE = Qt::UserRole + 7;
static constexpr int VERIFICATION_LEVEL_ROLE = Qt::UserRole + 8;
static constexpr int NODE_NAME_ROLE = Qt::UserRole + 9;
static constexpr int PURSUIT_COUNT_ROLE = Qt::UserRole + 11;
static constexpr int TASK_COUNT_ROLE = Qt::UserRole + 13;
static constexpr int ATTACHMENT_COUNT_ROLE = Qt::UserRole + 14;
static constexpr int DOMAIN_COLOR_ROLE = Qt::UserRole + 15;
static constexpr int DOMAIN_ICON_ROLE = Qt::UserRole + 16;
static constexpr int FLASHCARD_BACK_ROLE = Qt::UserRole + 1;
static constexpr int ATTACHMENT_STORAGE_NAME_ROLE = Qt::UserRole + 1;

static void setDomainItemAppearance(
    QStandardItem *item, const MetadataItem &domain, const bool navigationMode
) {
    item->setData(domain.color ? QVariant(*domain.color) : QVariant(), DOMAIN_COLOR_ROLE);
    item->setData(domain.icon ? QVariant(*domain.icon) : QVariant(), DOMAIN_ICON_ROLE);
    if (domain.color) {
        item->setForeground(QBrush(QColor(*domain.color)));
    } else {
        item->setData(QVariant(), Qt::ForegroundRole);
    }

    if (!domain.icon) {
        item->setIcon({});
        return;
    }

    QPixmap pixmap;
    pixmap.loadFromData(*domain.icon, "PNG");
    if (navigationMode && domain.color) {
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), QColor(*domain.color));
    }
    item->setIcon(QIcon(pixmap));
}

class DomainNodeDelegate final : public QStyledItemDelegate {
public:
    DomainNodeDelegate(
        std::function<void(int)> openComments,
        std::function<void()> indicatorPressed,
        QObject *parent
    ) : QStyledItemDelegate(parent),
        openComments(std::move(openComments)),
        indicatorPressed(std::move(indicatorPressed)) {}

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        if (parent()->property("navigationMode").toBool()) {
            size.setHeight(26);
        }
        return size;
    }

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
            QIcon icon;
            int width;
        };
        std::vector<Indicator> indicators;
        const QFontMetrics metrics(option.font);
        constexpr int iconSize = 14;
        constexpr int iconTextGap = 3;
        auto addIconIndicator = [&](const int role, const QString &iconPath,
                                    const QColor &color) {
            const int count = index.data(role).toInt();
            if (count > 0) {
                const QString text = QString::number(count);
                indicators.push_back({
                    text,
                    color,
                    QIcon(iconPath),
                    iconSize + iconTextGap + metrics.horizontalAdvance(text)
                });
            }
        };
        auto addTextIndicator = [&](const int role, const QString &symbol,
                                    const QColor &color) {
            const int count = index.data(role).toInt();
            if (count > 0) {
                const QString text = symbol + " " + QString::number(count);
                indicators.push_back({
                    text,
                    color,
                    {},
                    metrics.horizontalAdvance(text)
                });
            }
        };
        addIconIndicator(
            TASK_COUNT_ROLE, ":/assets/icons/task-white.png", QColor("#E0E0E0")
        );
        addIconIndicator(
            PURSUIT_COUNT_ROLE, ":/assets/icons/pursuit.png", QColor("#81C784")
        );
        addIconIndicator(
            FLASHCARD_COUNT_ROLE, ":/assets/icons/flashcards-white.png", QColor("#64B5F6")
        );
        addIconIndicator(
            ATTACHMENT_COUNT_ROLE,
            ":/assets/icons/attach-file.png",
            QColor("#BDBDBD")
        );
        addIconIndicator(
            COMMENT_COUNT_ROLE,
            ":/assets/icons/speech-bubble.png",
            QColor("#E0E0E0")
        );
        if (indicators.empty()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        const int gap = parent()->property("navigationMode").toBool() ? 6 : 10;
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
        painter->save();
        painter->setFont(option.font);
        int contentX = option.rect.left() + 4;
        const QIcon nodeIcon = index.data(Qt::DecorationRole).value<QIcon>();
        if (!nodeIcon.isNull()) {
            nodeIcon.paint(
                painter,
                QRect(
                    contentX,
                    option.rect.top() + (option.rect.height() - iconSize) / 2,
                    iconSize,
                    iconSize
                )
            );
            contentX += iconSize + 5;
        }
        const int contentRight = indicators.empty()
            ? option.rect.right() - 4
            : indicatorLeft - 7;
        const QString name = metrics.elidedText(
            index.data(Qt::DisplayRole).toString(),
            Qt::ElideRight,
            qMax(0, contentRight - contentX)
        );
        painter->setPen(
            option.state.testFlag(QStyle::State_Selected)
                ? option.palette.color(QPalette::HighlightedText)
                : option.palette.color(QPalette::Text)
        );
        painter->drawText(
            QRect(
                contentX,
                option.rect.top(),
                qMax(0, contentRight - contentX),
                option.rect.height()
            ),
            Qt::AlignLeft | Qt::AlignVCenter,
            name
        );

        int x = indicatorLeft;
        for (const Indicator &indicator : indicators) {
            int textX = x;
            if (!indicator.icon.isNull()) {
                indicator.icon.paint(
                    painter,
                    QRect(
                        x,
                        option.rect.top() + (option.rect.height() - iconSize) / 2,
                        iconSize,
                        iconSize
                    )
                );
                textX += iconSize + iconTextGap;
            }
            painter->setPen(indicator.color);
            painter->drawText(
                QRect(
                    textX,
                    option.rect.top(),
                    indicator.width - (textX - x),
                    option.rect.height()
                ),
                Qt::AlignLeft | Qt::AlignVCenter,
                indicator.text
            );
            x += indicator.width + gap;
        }
        painter->restore();
    }

    bool editorEvent(
        QEvent *event,
        QAbstractItemModel *,
        const QStyleOptionViewItem &option,
        const QModelIndex &index
    ) override {
        const int count = index.data(COMMENT_COUNT_ROLE).toInt();
        if (count <= 0
            || (event->type() != QEvent::MouseButtonPress
                && event->type() != QEvent::MouseButtonRelease)) {
            return false;
        }
        const auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const int width = 14 + 3
            + QFontMetrics(option.font).horizontalAdvance(QString::number(count));
        if (mouseEvent->button() != Qt::LeftButton
            || mouseEvent->position().x() < option.rect.right() - width - 5) {
            return false;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            indicatorPressed();
            return true;
        }
        openComments(index.data(Qt::UserRole).toInt());
        return true;
    }

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override {
        QStyledItemDelegate::initStyleOption(option, index);
        const QVariant foreground = index.data(Qt::ForegroundRole);
        if (foreground.isValid()) {
            option->palette.setBrush(QPalette::HighlightedText, foreground.value<QBrush>());
        }
    }

private:
    std::function<void(int)> openComments;
    std::function<void()> indicatorPressed;
};

static QString metadataName(const MetadataItem &item) {
    return item.displayName.value_or(item.name);
}

static QString metadataText(const std::vector<MetadataItem> &items, const int id) {
    for (const MetadataItem &item : items) {
        if (item.id == id) {
            return metadataName(item);
        }
    }
    return "#" + QString::number(id);
}

static QString knowledgeLevelText(
    const std::vector<KnowledgeLevel> &levels,
    const int id
) {
    for (const KnowledgeLevel &level : levels) {
        if (level.id == id) {
            return QString::number(level.id) + " – " + level.displayName;
        }
    }
    return "#" + QString::number(id);
}

struct KnowledgeLevelSelection {
    QWidget *widget;
    QButtonGroup *buttons;
};

static KnowledgeLevelSelection createKnowledgeLevelSelection(
    QWidget *parent,
    const std::vector<KnowledgeLevel> &levels,
    const std::optional<int> selectedId
) {
    auto *widget = new QWidget(parent);
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    auto *buttons = new QButtonGroup(widget);
    buttons->setExclusive(true);

    for (const KnowledgeLevel &level : levels) {
        auto *entry = new QWidget(widget);
        auto *entryLayout = new QVBoxLayout(entry);
        entryLayout->setContentsMargins(0, 0, 0, 0);
        entryLayout->setSpacing(2);

        auto *radio = new QRadioButton(
            QString::number(level.id) + " – " + level.displayName,
            entry
        );
        radio->setChecked(selectedId == std::optional<int>(level.id));
        buttons->addButton(radio, level.id);
        entryLayout->addWidget(radio);

        auto *description = new QLabel(level.description, entry);
        description->setWordWrap(true);
        description->setContentsMargins(26, 0, 0, 0);
        entryLayout->addWidget(description);
        layout->addWidget(entry);
    }

    return {widget, buttons};
}

static std::optional<int> selectedKnowledgeLevel(const QButtonGroup *buttons) {
    return buttons->checkedId() >= 0
        ? std::optional<int>(buttons->checkedId())
        : std::nullopt;
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

static void showKnowledgeLevelDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const int nodeId,
    const Metadata &metadata,
    const std::optional<int> knowledgeLevelId
) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Kenntnisstufe bearbeiten");
    dialog.setMinimumSize(520, 420);
    dialog.resize(620, 640);

    auto *layout = new QVBoxLayout(&dialog);
    auto *scrollArea = new QScrollArea(&dialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    const KnowledgeLevelSelection selection = createKnowledgeLevelSelection(
        scrollArea,
        metadata.knowledgeLevels,
        knowledgeLevelId
    );
    scrollArea->setWidget(selection.widget);
    layout->addWidget(scrollArea, 1);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    buttons->button(QDialogButtonBox::Save)->setText("Speichern");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    prepareFormDialog(dialog, *buttons, 520);

    if (execDialog(dialog) != QDialog::Accepted) {
        return;
    }

    const std::optional<int> updatedLevelId = selectedKnowledgeLevel(selection.buttons);
    if (updatedLevelId != knowledgeLevelId) {
        apiClient.updateDomainNodeField(
            nodeId,
            DomainNodeField::KnowledgeLevelId,
            jsonValue(updatedLevelId)
        );
    }
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
    item->setData(node.pursuitCount, PURSUIT_COUNT_ROLE);
    item->setData(node.taskCount, TASK_COUNT_ROLE);
    item->setData(static_cast<int>(node.comments.size()), COMMENT_COUNT_ROLE);
    setOptionalRole(item, KNOWLEDGE_LEVEL_ROLE, node.knowledgeLevelId);
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
    const int pursuitCount = item->data(PURSUIT_COUNT_ROLE).toInt();
    const int taskCount = item->data(TASK_COUNT_ROLE).toInt();

    QStringList details;
    if (taskCount > 0) {
        details.append("Aufgaben: " + QString::number(taskCount));
    }
    if (pursuitCount > 0) {
        details.append("Bestrebungen: " + QString::number(pursuitCount));
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
    form->addRow("Name", nameEdit);
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

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("Fertig");
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    prepareFormDialog(dialog, *buttons);
    execDialog(dialog, true);

    const QString updatedName = nameEdit->text().trimmed();
    if (!updatedName.isEmpty() && updatedName != name) {
        apiClient.updateDomainNodeField(
            nodeId, DomainNodeField::Name, updatedName
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
    dialog.resize(540, 340);

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
    buttons->button(QDialogButtonBox::Ok)->setText("Erstellen");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    prepareFormDialog(dialog, *buttons);

    if (execDialog(dialog) != QDialog::Accepted) {
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
    dialog.resize(540, 340);

    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    auto *frontEdit = new QPlainTextEdit(flashcard.front, &dialog);
    auto *backEdit = new QPlainTextEdit(flashcard.back, &dialog);
    form->addRow("Front", frontEdit);
    form->addRow("Back", backEdit);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("Fertig");
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    prepareFormDialog(dialog, *buttons);
    execDialog(dialog, true);

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

static bool showPursuitDialog(
    QWidget *parent,
    const Metadata &metadata,
    const Pursuit &pursuit,
    UpdatePursuitRequirements &result
) {
    for (const PursuitRequirement &requirement : pursuit.requirements) {
        switch (requirement.requirementTypeId) {
            case 1: result.knowledgeLevelId = requirement.knowledgeLevelId; break;
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

    auto *nodeTitle = new QLabel("Domain- und Aufschriebanforderungen", &dialog);
    QFont sectionFont = nodeTitle->font();
    sectionFont.setBold(true);
    nodeTitle->setFont(sectionFont);
    layout->addWidget(nodeTitle);
    auto *nodeForm = new QFormLayout();
    const KnowledgeLevelSelection knowledgeLevelSelection =
        createKnowledgeLevelSelection(
            &dialog,
            metadata.knowledgeLevels,
            result.knowledgeLevelId
        );
    auto *completenessCombo = new QComboBox(&dialog);
    auto *flashcardStatusCombo = new QComboBox(&dialog);
    auto *verificationStatusCombo = new QComboBox(&dialog);
    auto *verificationLevelCombo = new QComboBox(&dialog);
    fillMetadataCombo(completenessCombo, metadata.documentCompletenessLevels);
    fillMetadataCombo(flashcardStatusCombo, metadata.documentFlashcardStatuses);
    fillMetadataCombo(verificationStatusCombo, metadata.documentVerificationStatuses);
    fillMetadataCombo(verificationLevelCombo, metadata.documentVerificationLevels);
    selectMetadata(completenessCombo, result.completenessLevelId);
    selectMetadata(flashcardStatusCombo, result.flashcardStatusId);
    selectMetadata(verificationStatusCombo, result.verificationStatusId);
    selectMetadata(verificationLevelCombo, result.verificationLevelId);
    nodeForm->addRow("Kenntnisstufe", knowledgeLevelSelection.widget);
    nodeForm->addRow("Vollständigkeit", completenessCombo);
    nodeForm->addRow("Lernkartenstatus", flashcardStatusCombo);
    nodeForm->addRow("Verifizierungsstatus", verificationStatusCombo);
    nodeForm->addRow("Verifizierungsniveau", verificationLevelCombo);
    layout->addLayout(nodeForm);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    buttons->button(QDialogButtonBox::Ok)->setText("Fertig");
    prepareFormDialog(dialog, *buttons);

    if (execDialog(dialog, true) != QDialog::Accepted) {
        return false;
    }

    result.knowledgeLevelId = selectedKnowledgeLevel(knowledgeLevelSelection.buttons);
    result.completenessLevelId = selectedMetadata(completenessCombo);
    result.flashcardStatusId = selectedMetadata(flashcardStatusCombo);
    result.verificationStatusId = selectedMetadata(verificationStatusCombo);
    result.verificationLevelId = selectedMetadata(verificationLevelCombo);
    return true;
}

DomainView::DomainView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
    setObjectName("domainView");
    setStyleSheet(R"(
        QWidget#domainView {
            background: palette(base);
        }
        QWidget#domainView[navigationMode="true"] {
            background: transparent;
        }
        QTreeView#domainTree, QTreeView#pursuitTree {
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
        QTreeView#domainTree[navigationMode="true"] {
            background: transparent;
        }
        QTreeView#domainTree[navigationMode="true"]::item:hover {
            background: #303030;
        }
        QTreeView#domainTree[navigationMode="true"]::item:selected {
            background: #3D3D3D;
        }
        QWidget#editorHeader {
            background: rgba(255, 255, 255, 0.025);
        }
        QWidget#documentMetadataBar {
            background: #222222;
            border-top: 1px solid #383838;
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
    treeView->setItemDelegate(new DomainNodeDelegate(
        [this](const int nodeId) { showDomainNodeComments(nodeId); },
        [this] { commentIndicatorPressed = true; },
        treeView
    ));

    editorWidget = new QWidget(splitter);
    editorWidget->setObjectName("documentEditorPanel");
    editorWidget->setStyleSheet(R"(
        QWidget#documentEditorPanel {
            background: palette(base);
        }
        QWidget#documentEditor {
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

    documentEditor = new DocumentEditor(documentWidget);
    documentEditor->setObjectName("documentEditor");
    documentEditor->setMinimumWidth(400);
    autosaveTimer = new QTimer(this);
    autosaveTimer->setInterval(1000);
    autosaveTimer->setSingleShot(true);
    maximumAutosaveTimer = new QTimer(this);
    maximumAutosaveTimer->setInterval(5000);
    maximumAutosaveTimer->setSingleShot(true);
    documentLayout->addWidget(documentEditor, 1);

    documentMetadataWidget = new QWidget(editorWidget);
    documentMetadataWidget->setObjectName("documentMetadataBar");
    auto *documentMetadataLayout = new QHBoxLayout(documentMetadataWidget);
    documentMetadataLayout->setContentsMargins(10, 2, 10, 2);
    documentMetadataLayout->setSpacing(12);
    completenessLabel = new QLabel(documentMetadataWidget);
    completenessLabel->setTextFormat(Qt::RichText);
    completenessLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    completenessLabel->setStyleSheet("color: #A0A0A0; font-size: 11px;");
    saveStatus = new SaveStatus(documentMetadataWidget);
    documentMetadataLayout->addWidget(completenessLabel, 1);
    documentMetadataLayout->addWidget(saveStatus, 0, Qt::AlignRight | Qt::AlignVCenter);

    editorLayout->addWidget(documentWidget, 1);
    editorLayout->addWidget(documentMetadataWidget);
    editorWidget->hide();

    sidebarWidget = new QWidget(splitter);
    sidebarWidget->setObjectName("sidebarPanel");
    sidebarWidget->setFixedWidth(300);
    sidebarWidget->setStyleSheet(R"(
        QWidget#sidebarPanel, QWidget#sidebarContent,
        QWidget#taskPanel, QWidget#pursuitPanel, QWidget#flashcardsPanel,
        QWidget#attachmentsPanel,
        QScrollArea#sidebarScrollArea {
            background: #1C1C1C;
            border: none;
        }
        QWidget#sidebarList {
            background: #1C1C1C;
            border-left: 1px solid #383838;
        }
        QPushButton#taskSectionButton,
        QPushButton#pursuitSectionButton,
        QPushButton#flashcardSectionButton,
        QPushButton#attachmentSectionButton {
            min-height: 31px;
            padding: 0 10px;
            color: #E5E5E5;
            background: #262626;
            border: none;
            border-bottom: 1px solid #383838;
            border-radius: 0;
            text-align: left;
            font-size: 12px;
            font-weight: 600;
        }
        QWidget#flashcardSectionHeader {
            min-height: 31px;
            background: #262626;
            border-bottom: 1px solid #383838;
        }
        QPushButton#flashcardSectionButton {
            background: transparent;
            border-bottom: none;
        }
        QLabel#flashcardSectionStatus {
            background: transparent;
            padding-right: 10px;
            font-size: 10px;
        }
        QPushButton#taskSectionButton:hover,
        QPushButton#pursuitSectionButton:hover,
        QPushButton#attachmentSectionButton:hover,
        QPushButton#flashcardSectionButton:hover {
            background: #303030;
        }
        QPushButton#taskSectionButton:focus,
        QPushButton#pursuitSectionButton:focus,
        QPushButton#attachmentSectionButton:focus,
        QPushButton#flashcardSectionButton:focus {
            border-left: 2px solid #708FA6;
        }
        QTreeView#pursuitTree {
            background: #1C1C1C;
            border: none;
            outline: none;
        }
        QTreeView#pursuitTree::item {
            border: none;
            padding: 2px 3px;
        }
        QListView#taskList, QListView#flashcardList, QListView#attachmentList {
            background: #1C1C1C;
            border: none;
            outline: none;
        }
        QListView#taskList::item, QListView#flashcardList::item,
        QListView#attachmentList::item {
            border: none;
            padding: 2px 3px;
        }
        QTreeView#pursuitTree::item:hover,
        QListView#taskList::item:hover, QListView#flashcardList::item:hover,
        QListView#attachmentList::item:hover {
            background: #303030;
        }
        QTreeView#pursuitTree::item:selected,
        QListView#taskList::item:selected, QListView#flashcardList::item:selected,
        QListView#attachmentList::item:selected {
            background: #394650;
            color: #E5E5E5;
        }
        QScrollBar:vertical {
            width: 10px;
            background: #1C1C1C;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            min-height: 28px;
            margin: 2px;
            background: #484848;
            border-radius: 3px;
        }
        QScrollBar::handle:vertical:hover {
            background: #5A5A5A;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            height: 0;
            background: transparent;
        }
    )");
    auto *sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    auto *sidebarScrollArea = new QScrollArea(sidebarWidget);
    sidebarScrollArea->setObjectName("sidebarScrollArea");
    sidebarScrollArea->setWidgetResizable(true);
    sidebarScrollArea->setFrameShape(QFrame::NoFrame);
    sidebarScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebarScrollArea->viewport()->setStyleSheet(
        "QWidget#qt_scrollarea_viewport { background: #1C1C1C; }"
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
    sidebarListLayout->setContentsMargins(0, 0, 0, 0);
    sidebarListLayout->setSpacing(0);

    auto *taskSectionButton = new QPushButton("▾  Aufgaben", sidebarList);
    taskSectionButton->setObjectName("taskSectionButton");
    taskSectionButton->setCheckable(true);
    taskSectionButton->setChecked(true);

    auto *taskPanel = new QWidget(sidebarList);
    taskPanel->setObjectName("taskPanel");
    taskPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *taskPanelLayout = new QVBoxLayout(taskPanel);
    taskPanelLayout->setContentsMargins(8, 4, 8, 5);
    taskPanelLayout->setSpacing(2);

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
    taskListView->setItemDelegate(new CommentIndicatorDelegate(
        [this](const int taskId) { showTaskComments(taskId); },
        {},
        taskListView
    ));
    taskPanelLayout->addWidget(taskListView);

    auto *pursuitPanel = new QWidget(sidebarList);
    pursuitPanel->setObjectName("pursuitPanel");
    pursuitPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *pursuitLayout = new QVBoxLayout(pursuitPanel);
    pursuitLayout->setContentsMargins(8, 4, 8, 5);
    pursuitLayout->setSpacing(2);

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
    pursuitTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    pursuitModel = new QStandardItemModel(pursuitTreeView);
    pursuitTreeView->setModel(pursuitModel);
    pursuitTreeView->setItemDelegate(new CommentIndicatorDelegate(
        [this](const int pursuitId) { showPursuitComments(pursuitId); },
        [this] { commentIndicatorPressed = true; },
        pursuitTreeView
    ));
    pursuitLayout->addWidget(pursuitTreeView);

    auto *flashcardPanel = new QWidget(sidebarList);
    flashcardPanel->setObjectName("flashcardsPanel");
    flashcardPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *flashcardPanelLayout = new QVBoxLayout(flashcardPanel);
    flashcardPanelLayout->setContentsMargins(8, 4, 8, 5);
    flashcardPanelLayout->setSpacing(2);

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
    flashcardListView->setItemDelegate(new CommentIndicatorDelegate(
        [this](const int flashcardId) { showFlashcardComments(flashcardId); },
        [this] { commentIndicatorPressed = true; },
        flashcardListView
    ));
    flashcardPanelLayout->addWidget(flashcardListView);

    auto *attachmentSectionButton = new QPushButton("▾  Anhänge", sidebarList);
    attachmentSectionButton->setObjectName("attachmentSectionButton");
    attachmentSectionButton->setCheckable(true);
    attachmentSectionButton->setChecked(true);

    auto *attachmentPanel = new QWidget(sidebarList);
    attachmentPanel->setObjectName("attachmentsPanel");
    attachmentPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *attachmentPanelLayout = new QVBoxLayout(attachmentPanel);
    attachmentPanelLayout->setContentsMargins(8, 4, 8, 5);
    attachmentPanelLayout->setSpacing(2);

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
    sidebarListLayout->addWidget(flashcardSectionHeader);
    sidebarListLayout->addWidget(flashcardPanel);
    sidebarListLayout->addWidget(attachmentSectionButton);
    sidebarListLayout->addWidget(attachmentPanel);
    sidebarContentLayout->addWidget(sidebarList);
    sidebarContentLayout->addStretch();
    sidebarScrollArea->setWidget(sidebarContent);
    sidebarLayout->addWidget(sidebarScrollArea);
    sidebarWidget->hide();

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
    splitter->addWidget(sidebarWidget);
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
    connect(&apiClient, &ApiClient::domainUpdated, this,
            [this](const MetadataItem &domain) {
                QStandardItem *item = findDomainItem(domain.id);
                if (!item) {
                    return;
                }
                setDomainItemAppearance(item, domain, property("navigationMode").toBool());
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
                        break;
                    case DomainNodeField::KnowledgeLevelId:
                        item->setData(value.isNull() ? QVariant() : value.toInt(),
                                      KNOWLEDGE_LEVEL_ROLE);
                        treeView->update(item->index());
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
                item->setIcon(QIcon(":/assets/icons/file.png"));
                treeView->setCurrentIndex(item->index());
                currentDocumentNodeId = document.domainNodeId;
                savedContent = document.content;
                documentEditor->setMarkdown(document.content);
                updateFlashcardStatus(document.flashcardStatusId);
                updateDocumentStatus(
                    document.completenessLevelId,
                    document.verificationStatusId,
                    document.verificationLevelId
                );
                saveStatus->setState(SaveStatus::Saved);
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
            });
    connect(&apiClient, &ApiClient::documentCreated, this,
            [this](const Document &document) {
                QStandardItem *item = findDomainNodeItem(document.domainNodeId);
                item->setData(true, HAS_DOCUMENT_ROLE);
                item->setIcon(QIcon(":/assets/icons/file.png"));
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
    connect(&apiClient, &ApiClient::documentVersionRestored, this,
            [this](const Document &document) {
                if (document.domainNodeId != currentDocumentNodeId) {
                    return;
                }

                autosaveTimer->stop();
                maximumAutosaveTimer->stop();
                savedContent = document.content;
                documentEditor->setMarkdown(document.content);
                saveStatus->setState(SaveStatus::Saved);
            });
    connect(treeView, &QTreeView::customContextMenuRequested, this,
            [this](const QPoint &position) {
                showDomainNodeContextMenu(position);
            });
    connect(treeView, &QTreeView::clicked, this, [this](const QModelIndex &index) {
        if (commentIndicatorPressed) {
            commentIndicatorPressed = false;
            return;
        }
        const QModelIndex nodeIndex = index.sibling(index.row(), 0);
        if (!nodeIndex.data(Qt::UserRole).isValid()) {
            requestDocumentChange(-1);
            return;
        }

        if (nodeOpenHandler) {
            nodeOpenHandler(
                nodeIndex.data(Qt::UserRole).toInt(),
                nodeIndex.data(NODE_NAME_ROLE).toString()
            );
            return;
        }

        requestDocumentChange(
            nodeIndex.data(Qt::UserRole).toInt(),
            nodeIndex.data(HAS_DOCUMENT_ROLE).toBool()
        );
    });
    connect(flashcardListView, &QListView::clicked, this,
            [this](const QModelIndex &index) {
                if (commentIndicatorPressed) {
                    commentIndicatorPressed = false;
                    return;
                }
                editFlashcard(index.row());
            });
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
                QAction *commentsAction = menu.addAction(
                    QIcon(":/assets/icons/speech-bubble.png"),
                    "Kommentare anzeigen"
                );
                QAction *historyAction = menu.addAction(
                    QIcon(":/assets/icons/version_history.png"),
                    "Versionshistorie"
                );
                menu.addSeparator();
                QAction *deleteAction = menu.addAction(contextMenuDeleteIcon(), "Löschen");
                QAction *selectedAction = menu.exec(
                    taskListView->viewport()->mapToGlobal(position)
                );
                const int taskId = index.data(Qt::UserRole).toInt();
                if (selectedAction == editAction) {
                    editTask(taskId);
                } else if (selectedAction == commentsAction) {
                    showTaskComments(taskId);
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
                QAction *commentsAction = menu.addAction(
                    QIcon(":/assets/icons/speech-bubble.png"),
                    "Kommentare anzeigen"
                );
                QAction *historyAction = menu.addAction(
                    QIcon(":/assets/icons/version_history.png"),
                    "Versionshistorie"
                );
                menu.addSeparator();
                QAction *deleteAction = menu.addAction(contextMenuDeleteIcon(), "Löschen");
                QAction *selectedAction = menu.exec(
                    flashcardListView->viewport()->mapToGlobal(position)
                );
                if (selectedAction == commentsAction) {
                    showFlashcardComments(index.data(Qt::UserRole).toInt());
                } else if (selectedAction == deleteAction) {
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

                currentFlashcards = flashcards;
                flashcardModel->removeRows(0, flashcardModel->rowCount());
                for (const Flashcard &flashcard : flashcards) {
                    auto *item = new QStandardItem(flashcard.front);
                    item->setData(flashcard.id, Qt::UserRole);
                    item->setData(flashcard.back, FLASHCARD_BACK_ROLE);
                    item->setData(flashcard.id, COMMENT_ENTITY_ID_ROLE);
                    item->setData(
                        static_cast<int>(flashcard.comments.size()),
                        COMMENT_COUNT_ROLE
                    );
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
                    contextMenuDeleteIcon(),
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
    connect(pursuitTreeView, &QTreeView::clicked, this,
            [this](const QModelIndex &index) {
                if (commentIndicatorPressed) {
                    commentIndicatorPressed = false;
                    return;
                }
                editPursuit(index);
            });
    connect(pursuitTreeView, &QTreeView::customContextMenuRequested, this,
            [this](const QPoint &position) {
                const QModelIndex index = pursuitTreeView->indexAt(position);
                if (!index.isValid() || !index.data(COMMENT_ENTITY_ID_ROLE).isValid()) {
                    return;
                }
                QMenu menu(pursuitTreeView);
                QAction *editAction = menu.addAction(
                    QIcon(":/assets/icons/edit.png"),
                    "Bearbeiten"
                );
                QAction *commentsAction = menu.addAction(
                    QIcon(":/assets/icons/speech-bubble.png"),
                    "Kommentare anzeigen"
                );
                QAction *selectedAction = menu.exec(
                    pursuitTreeView->viewport()->mapToGlobal(position)
                );
                if (selectedAction == editAction) {
                    editPursuit(index);
                } else if (selectedAction == commentsAction) {
                    showPursuitComments(index.data(COMMENT_ENTITY_ID_ROLE).toInt());
                }
            });
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
    connect(documentEditor, &DocumentEditor::markdownChanged, this, [this] {
        if (currentDocumentNodeId == -1) {
            return;
        }

        if (documentEditor->markdown() == savedContent) {
            autosaveTimer->stop();
            maximumAutosaveTimer->stop();
            saveStatus->setState(SaveStatus::Saved);
        } else {
            saveStatus->setState(SaveStatus::Saving);
            autosaveTimer->start();
            if (!maximumAutosaveTimer->isActive()) {
                maximumAutosaveTimer->start();
            }
        }
    });
    connect(autosaveTimer, &QTimer::timeout, this, &DomainView::startSave);
    connect(maximumAutosaveTimer, &QTimer::timeout, this, &DomainView::startSave);
    connect(&apiClient, &ApiClient::metadataReceived, this, [this] {
        apiClient.getDomainTree();
    });

}

void DomainView::saveBeforeClose(std::function<void()> continuation) {
    closeContinuation = std::move(continuation);
    autosaveTimer->stop();
    maximumAutosaveTimer->stop();
    finishPendingActions();
}

void DomainView::refreshTasks() {
    apiClient.getTasks();
}

void DomainView::setNodeOpenHandler(NodeOpenHandler handler) {
    nodeOpenHandler = std::move(handler);
    setProperty("navigationMode", true);
    treeView->setProperty("navigationMode", true);
    style()->unpolish(this);
    style()->polish(this);
    treeView->style()->unpolish(treeView);
    treeView->style()->polish(treeView);

    QFont treeFont(QStringLiteral("Segoe UI"));
    treeFont.setPixelSize(13);
    treeView->setFont(treeFont);
    QPalette treePalette = treeView->palette();
    const QColor textColor("#F4F4F4");
    treePalette.setColor(QPalette::Text, textColor);
    treePalette.setColor(QPalette::HighlightedText, textColor);
    treeView->setPalette(treePalette);

    treeView->setMinimumWidth(0);
    treeView->setMaximumWidth(QWIDGETSIZE_MAX);
    treeView->setIndentation(18);
    treeView->setIconSize(QSize(14, 14));
    treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    treeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    editorWidget->hide();
    sidebarWidget->hide();

    connect(treeView, &QTreeView::expanded, this,
            [this] { updateNavigationTreeHeight(); });
    connect(treeView, &QTreeView::collapsed, this,
            [this] { updateNavigationTreeHeight(); });
    connect(model, &QAbstractItemModel::rowsInserted, this,
            [this] { updateNavigationTreeHeight(); });
    connect(model, &QAbstractItemModel::rowsRemoved, this,
            [this] { updateNavigationTreeHeight(); });
    connect(model, &QAbstractItemModel::modelReset, this,
            [this] { updateNavigationTreeHeight(); });
    updateNavigationTreeHeight();
}

void DomainView::setBeforeNodeActionHandler(BeforeNodeActionHandler handler) {
    beforeNodeActionHandler = std::move(handler);
}

void DomainView::showNode(const int nodeId) {
    nodeOnly = true;
    initialNodeId = nodeId;
    treeView->hide();
    apiClient.getDomainTree();
    apiClient.getTasks();
}

void DomainView::prepareNodeAction(
    const int nodeId,
    std::function<void()> continuation
) {
    if (beforeNodeActionHandler) {
        beforeNodeActionHandler(nodeId, std::move(continuation));
    } else {
        saveBeforeClose(std::move(continuation));
    }
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
    maximumAutosaveTimer->stop();
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
    attachmentModel->removeRows(0, attachmentModel->rowCount());
    taskModel->removeRows(0, taskModel->rowCount());
    pursuitModel->removeRows(0, pursuitModel->rowCount());
    currentPursuits.clear();
    currentFlashcards.clear();
    renderTasks();
    noPursuitsLabel->show();
    pursuitTreeView->hide();
    documentMetadataWidget->hide();
    editorWidget->setVisible(nodeId != -1);
    sidebarWidget->setVisible(nodeId != -1);
    documentWidget->setVisible(hasDocument);
    updateFlashcardView();
    renderAttachments({});

    if (nodeId != -1) {
        updateFlashcardStatus(optionalRole(
            findDomainNodeItem(nodeId)->index(), FLASHCARD_STATUS_ROLE
        ));
        apiClient.getNodeFlashcards(nodeId);
        apiClient.getNodeAttachments(nodeId);
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
        || documentEditor->markdown() == savedContent) {
        return;
    }

    autosaveTimer->stop();
    maximumAutosaveTimer->stop();
    saveInProgress = true;
    savingDocumentNodeId = currentDocumentNodeId;
    savingContent = documentEditor->markdown();
    saveStatus->setState(SaveStatus::Saving);
    apiClient.updateDocument(currentDocumentNodeId, savingContent);
}

void DomainView::finishPendingActions() {
    if (saveInProgress) {
        return;
    }

    if (currentDocumentNodeId != -1 && documentEditor->markdown() != savedContent) {
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

void DomainView::renderDomainTree(
    const std::vector<MetadataItem> &domains,
    const std::vector<DomainNode> &nodes
) {
    const bool navigationMode = property("navigationMode").toBool();
    QSet<int> expandedDomains;
    QSet<int> expandedNodes;
    if (navigationMode) {
        std::function<void(const QModelIndex &)> rememberExpanded;
        rememberExpanded = [&](const QModelIndex &parent) {
            for (int row = 0; row < model->rowCount(parent); ++row) {
                const QModelIndex index = model->index(row, 0, parent);
                if (treeView->isExpanded(index)) {
                    const QVariant domainId = index.data(DOMAIN_ID_ROLE);
                    if (domainId.isValid()) {
                        expandedDomains.insert(domainId.toInt());
                    } else {
                        expandedNodes.insert(index.data(Qt::UserRole).toInt());
                    }
                }
                rememberExpanded(index);
            }
        };
        rememberExpanded({});
    }

    domainNodes = nodes;
    model->clear();
    model->setColumnCount(1);

    QModelIndexList itemsToExpand;
    for (const MetadataItem &domain : domains) {
        auto *domainItem = new QStandardItem(domain.name);
        if (!navigationMode) {
            QFont font = domainItem->font();
            font.setBold(true);
            domainItem->setFont(font);
        }
        domainItem->setData(domain.id, DOMAIN_ID_ROLE);
        setDomainItemAppearance(domainItem, domain, navigationMode);
        model->appendRow(domainItem);
        if (expandedDomains.contains(domain.id)) {
            itemsToExpand.append(domainItem->index());
        }

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
                    item->setIcon(QIcon(":/assets/icons/file.png"));
                }
                parentItem->appendRow(item);
                refreshDomainNodeText(item);
                if (expandedNodes.contains(node.id)) {
                    itemsToExpand.append(item->index());
                }
                addNodes(node.id, item);
            }
        };

        addNodes(std::nullopt, domainItem);
    }

    treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    if (navigationMode) {
        const QSignalBlocker blocker(treeView);
        for (const QModelIndex &index : itemsToExpand) {
            treeView->expand(index);
        }
    } else {
        treeView->collapseAll();
    }

    if (nodeOnly && initialNodeId != -1) {
        QStandardItem *item = findDomainNodeItem(initialNodeId);
        requestDocumentChange(
            initialNodeId,
            item->data(HAS_DOCUMENT_ROLE).toBool()
        );
    }
    updateNavigationTreeHeight();
}

void DomainView::updateNavigationTreeHeight() {
    if (!property("navigationMode").toBool()) {
        return;
    }

    std::function<int(const QModelIndex &)> visibleHeight;
    visibleHeight = [this, &visibleHeight](const QModelIndex &parent) {
        int height = 0;
        for (int row = 0; row < model->rowCount(parent); ++row) {
            const QModelIndex index = model->index(row, 0, parent);
            const int rowHeight = treeView->sizeHintForIndex(index).height();
            height += rowHeight > 0 ? rowHeight : treeView->fontMetrics().height() + 4;
            if (treeView->isExpanded(index)) {
                height += visibleHeight(index);
            }
        }
        return height;
    };

    const int height = visibleHeight({});
    treeView->setFixedHeight(height);
    setFixedHeight(height);
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
        QAction *colorAction = menu.addAction(property("navigationMode").toBool()
            ? "Fachfarbe auswählen" : "Schriftfarbe auswählen");
        QAction *iconAction = menu.addAction("Icon auswählen");
        QAction *removeIconAction = index.data(DOMAIN_ICON_ROLE).isValid()
            ? menu.addAction("Icon entfernen")
            : nullptr;

        QAction *selectedAction = menu.exec(treeView->viewport()->mapToGlobal(position));
        const int domainId = index.data(DOMAIN_ID_ROLE).toInt();
        const std::optional<QString> color = index.data(DOMAIN_COLOR_ROLE).isValid()
            ? std::optional(index.data(DOMAIN_COLOR_ROLE).toString())
            : std::nullopt;
        const std::optional<QByteArray> icon = index.data(DOMAIN_ICON_ROLE).isValid()
            ? std::optional(index.data(DOMAIN_ICON_ROLE).toByteArray())
            : std::nullopt;

        if (selectedAction == createAction) {
            const auto input = showTextInputDialog(
                treeView,
                "Root-Knoten hinzufügen",
                "Name"
            );
            const QString name = input ? input->trimmed() : QString();

            if (!name.isEmpty()) {
                apiClient.createRootDomainNode(domainId, name);
            }
        } else if (selectedAction == colorAction) {
            const QColor selectedColor = QColorDialog::getColor(
                color ? QColor(*color) : treeView->palette().color(QPalette::Text),
                treeView,
                "Schriftfarbe auswählen"
            );
            if (selectedColor.isValid()) {
                apiClient.updateDomain(domainId, selectedColor.name(), icon);
            }
        } else if (selectedAction == iconAction) {
            const QString fileName = QFileDialog::getOpenFileName(
                treeView,
                "Icon auswählen",
                {},
                "PNG-Bilder (*.png)"
            );
            if (!fileName.isEmpty()) {
                QFile file(fileName);
                if (file.open(QIODevice::ReadOnly)) {
                    apiClient.updateDomain(domainId, color, file.readAll());
                }
            }
        } else if (removeIconAction && selectedAction == removeIconAction) {
            apiClient.updateDomain(domainId, color, std::nullopt);
        }

        return;
    }

    QMenu menu(treeView);
    QMenu *addMenu = menu.addMenu("Hinzufügen");
    QAction *createAction = addMenu->addAction(
        QIcon(":/assets/icons/node.png"),
        "Unterknoten"
    );
    QAction *createTaskAction = addMenu->addAction(
        QIcon(":/assets/icons/task-white.png"),
        "Aufgabe"
    );
    QAction *createPursuitAction = addMenu->addAction(
        QIcon(":/assets/icons/pursuit.png"),
        "Bestrebung"
    );
    QAction *createFlashcardAction = addMenu->addAction(
        QIcon(":/assets/icons/flashcards-white.png"),
        "Lernkarte"
    );
    QAction *createAttachmentAction = addMenu->addAction(
        QIcon(":/assets/icons/attach-file.png"),
        "Anhang"
    );
    QAction *editAction = menu.addAction(
        QIcon(":/assets/icons/edit.png"),
        "Bearbeiten"
    );
    QAction *knowledgeLevelAction = menu.addAction(QIcon(":/assets/icons/knowledge.png"),"Kenntnisstufe bearbeiten");
    QAction *commentsAction = menu.addAction(
        QIcon(":/assets/icons/speech-bubble.png"),
        "Kommentare anzeigen"
    );
    QAction *createDocumentAction = nullptr;
    QAction *deleteDocumentAction = nullptr;
    QAction *documentHistoryAction = nullptr;
    const bool hasDocument = index.data(HAS_DOCUMENT_ROLE).toBool();
    if (!hasDocument) {
        createDocumentAction = menu.addAction(
            QIcon(":/assets/icons/add.png"),
            "Aufschrieb erstellen"
        );
    }
    menu.addSeparator();
    QAction *historyAction = menu.addAction(
        QIcon(":/assets/icons/version_history.png"),
        "Versionshistorie"
    );
    if (hasDocument) {
        documentHistoryAction = menu.addAction(
            QIcon(":/assets/icons/version_history.png"),
            "Aufschrieb-Versionshistorie"
        );
    }
    menu.addSeparator();
    if (hasDocument) {
        deleteDocumentAction = menu.addAction(contextMenuDeleteIcon(), "Aufschrieb löschen");
    }
    QAction *deleteAction = menu.addAction(contextMenuDeleteIcon(), "Löschen");
    QAction *selectedAction = menu.exec(treeView->viewport()->mapToGlobal(position));
    const int nodeId = index.data(Qt::UserRole).toInt();

    if (selectedAction == createAction) {
        const auto input = showTextInputDialog(
            treeView,
            "Unterknoten hinzufügen",
            "Name"
        );
        const QString name = input ? input->trimmed() : QString();

        if (!name.isEmpty()) {
            apiClient.createDomainNode(nodeId, name);
        }
    } else if (selectedAction == editAction) {
        showDomainNodeDialog(
            treeView,
            apiClient,
            nodeId,
            apiClient.metadata(),
            index.data(NODE_NAME_ROLE).toString(),
            index.data(HAS_DOCUMENT_ROLE).toBool(),
            optionalRole(index, COMPLETENESS_LEVEL_ROLE),
            optionalRole(index, FLASHCARD_STATUS_ROLE),
            optionalRole(index, VERIFICATION_STATUS_ROLE),
            optionalRole(index, VERIFICATION_LEVEL_ROLE)
        );
    } else if (selectedAction == knowledgeLevelAction) {
        showKnowledgeLevelDialog(
            treeView,
            apiClient,
            nodeId,
            apiClient.metadata(),
            optionalRole(index, KNOWLEDGE_LEVEL_ROLE)
        );
    } else if (selectedAction == commentsAction) {
        showDomainNodeComments(nodeId);
    } else if (createDocumentAction && selectedAction == createDocumentAction) {
        apiClient.createDocument(nodeId);
    } else if (deleteDocumentAction && selectedAction == deleteDocumentAction
               && QMessageBox::question(
                   treeView,
                   "Aufschrieb löschen",
                   "Aufschrieb dieses Domain Nodes löschen?"
               ) == QMessageBox::Yes) {
        prepareNodeAction(
            nodeId,
            [this, nodeId] { apiClient.deleteDocument(nodeId); }
        );
    } else if (documentHistoryAction && selectedAction == documentHistoryAction) {
        const QString title = "Aufschrieb-Versionshistorie – "
            + index.data(NODE_NAME_ROLE).toString();
        prepareNodeAction(nodeId, [this, nodeId, title] {
            showDocumentVersionHistoryDialog(treeView, apiClient, nodeId, title);
        });
    } else if (selectedAction == createTaskAction) {
        createTask(nodeId);
    } else if (selectedAction == createPursuitAction) {
        createPursuit(nodeId);
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
        prepareNodeAction(
            nodeId,
            [this, nodeId] { apiClient.deleteDomainNode(nodeId); }
        );
    }
}

void DomainView::createFlashcard(const int nodeId) {
    QString front;
    QString back;
    if (showFlashcardDialog(this, "Lernkarte erstellen", front, back)) {
        apiClient.createFlashcard(nodeId, front, back);
    }
}

void DomainView::showDomainNodeComments(const int nodeId) {
    const auto node = std::find_if(
        domainNodes.begin(), domainNodes.end(),
        [nodeId](const DomainNode &item) { return item.id == nodeId; }
    );
    if (node == domainNodes.end()) {
        return;
    }
    showCommentsDialog(
        treeView,
        apiClient,
        node->id,
        "Kommentare – " + node->name,
        node->comments
    );
    if (QStandardItem *item = findDomainNodeItem(nodeId)) {
        item->setData(static_cast<int>(node->comments.size()), COMMENT_COUNT_ROLE);
    }
}

void DomainView::showTaskComments(const int taskId) {
    const auto task = std::find_if(
        currentTasks.begin(), currentTasks.end(),
        [taskId](const Task &item) { return item.id == taskId; }
    );
    if (task == currentTasks.end()) {
        return;
    }
    showCommentsDialog(
        taskListView,
        apiClient,
        task->id,
        "Kommentare – " + task->name,
        task->comments
    );
    renderTasks();
}

void DomainView::showFlashcardComments(const int flashcardId) {
    const auto flashcard = std::find_if(
        currentFlashcards.begin(), currentFlashcards.end(),
        [flashcardId](const Flashcard &item) { return item.id == flashcardId; }
    );
    if (flashcard == currentFlashcards.end()) {
        return;
    }
    showCommentsDialog(
        flashcardListView,
        apiClient,
        flashcard->id,
        "Kommentare – Lernkarte",
        flashcard->comments
    );
    for (int row = 0; row < flashcardModel->rowCount(); ++row) {
        QStandardItem *item = flashcardModel->item(row);
        if (item->data(Qt::UserRole).toInt() == flashcardId) {
            item->setData(
                static_cast<int>(flashcard->comments.size()),
                COMMENT_COUNT_ROLE
            );
            break;
        }
    }
}

void DomainView::showPursuitComments(const int pursuitId) {
    const auto pursuit = std::find_if(
        currentPursuits.begin(), currentPursuits.end(),
        [pursuitId](const Pursuit &item) { return item.id == pursuitId; }
    );
    if (pursuit == currentPursuits.end()) {
        return;
    }
    showCommentsDialog(
        pursuitTreeView,
        apiClient,
        pursuit->id,
        "Kommentare – " + pursuit->name,
        pursuit->comments
    );
    renderPursuits();
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

void DomainView::renderTasks() {
    taskModel->clear();

    for (const Task &task : currentTasks) {
        if (task.domainNodeId != currentNodeId) {
            continue;
        }

        auto *item = new QStandardItem(task.name);
        item->setData(task.id, Qt::UserRole);
        item->setData(task.id, COMMENT_ENTITY_ID_ROLE);
        item->setData(static_cast<int>(task.comments.size()), COMMENT_COUNT_ROLE);
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
        pursuitItem->setData(pursuit.id, COMMENT_ENTITY_ID_ROLE);
        pursuitItem->setData(
            static_cast<int>(pursuit.comments.size()),
            COMMENT_COUNT_ROLE
        );
        pursuitItem->setIcon(QIcon(":/assets/icons/pursuit.png"));
        pursuitItem->setSizeHint(QSize(0, 24));

        for (const PursuitRequirement &requirement : pursuit.requirements) {
            QString text;
            switch (requirement.requirementTypeId) {
                case 1:
                    text = "Kenntnisstufe: " + knowledgeLevelText(
                        apiClient.metadata().knowledgeLevels,
                        *requirement.knowledgeLevelId
                    );
                    break;
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
    showTaskCreateDialog(this, apiClient, {}, {}, nodeId);
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
    const auto input = showTextInputDialog(
        this,
        "Bestrebung hinzufügen",
        "Name"
    );
    const QString name = input ? input->trimmed() : QString();
    if (!name.isEmpty()) {
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
            this, apiClient.metadata(), pursuit, requirements
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
    QString completenessText;
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
        completenessText = "Der Aufschrieb ist " + highlighted(phrase, color) + ".";
    } else {
        completenessText = "Die Vollständigkeit des Aufschriebs wurde "
            + highlighted("noch nicht bewertet", "#9E9E9E") + ".";
    }

    const MetadataItem *verificationStatus = findMetadataItem(
        apiClient.metadata().documentVerificationStatuses,
        verificationStatusId
    );
    QString verificationText;
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

        verificationText = "Der Aufschrieb ist "
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
            verificationText += " und wurde " + highlighted(levelPhrase, levelColor);
        }
        verificationText += ".";
    } else {
        verificationText = "Die Verifizierung des Aufschriebs wurde "
            + highlighted("noch nicht bewertet", "#9E9E9E") + ".";
    }

    completenessLabel->setText(completenessText + " " + verificationText);
    completenessLabel->show();
    documentMetadataWidget->show();
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
