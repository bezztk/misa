#include "VersionHistoryDialog.h"
#include "DialogUtils.h"

#include "../api/ApiClient.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QPushButton>

static QString metadataName(
    const std::vector<MetadataItem> &items,
    const QJsonValue &value
) {
    if (value.isNull()) {
        return "—";
    }
    const int id = value.toInt();
    for (const MetadataItem &item : items) {
        if (item.id == id) {
            return item.displayName.value_or(item.name);
        }
    }
    return QString::number(id);
}

static QString knowledgeLevelName(
    const std::vector<KnowledgeLevel> &levels,
    const QJsonValue &value
) {
    if (value.isNull()) {
        return "—";
    }
    const int id = value.toInt();
    for (const KnowledgeLevel &level : levels) {
        if (level.id == id) {
            return QString::number(level.id) + " – " + level.displayName;
        }
    }
    return QString::number(id);
}

static QString fieldName(const QString &name, const bool idea) {
    if (name == "name") return "Name";
    if (name == "title") return "Titel";
    if (name == "content") return "Beschreibung";
    if (name == "phase_id") return "Phase";
    if (name == "knowledge_level_id") return "Kenntnisstufe";
    if (name == "completeness_level_id") return "Vollständigkeit";
    if (name == "flashcard_status_id") return "Lernkartenstatus";
    if (name == "verification_status_id") return "Verifizierungsstatus";
    if (name == "verification_level_id") return "Verifizierungsniveau";
    if (name == "state_id") return idea ? "Ideenstatus" : "Status";
    if (name == "priority_id") return "Priorität";
    if (name == "due_at") return "Fälligkeit";
    if (name == "front") return "Vorderseite";
    if (name == "back") return "Rückseite";
    if (name == "personal_relevance_id") return "Persönliche Relevanz";
    if (name == "lasting_impact_id") return "Langfristige Auswirkung";
    return name;
}

static QString fieldValue(
    const Metadata &metadata,
    const QString &field,
    const QJsonValue &value,
    const bool idea
) {
    if (field == "knowledge_level_id") {
        return knowledgeLevelName(metadata.knowledgeLevels, value);
    }
    if (field == "completeness_level_id") {
        return metadataName(metadata.documentCompletenessLevels, value);
    }
    if (field == "flashcard_status_id") {
        return metadataName(metadata.documentFlashcardStatuses, value);
    }
    if (field == "verification_status_id") {
        return metadataName(metadata.documentVerificationStatuses, value);
    }
    if (field == "verification_level_id") {
        return metadataName(metadata.documentVerificationLevels, value);
    }
    if (field == "state_id") {
        return metadataName(
            idea ? metadata.ideaStates : metadata.systemTaskStates,
            value
        );
    }
    if (field == "phase_id") {
        return metadataName(metadata.ideaPhases, value);
    }
    if (field == "priority_id") {
        return metadataName(metadata.systemTaskPriorities, value);
    }
    if (field == "personal_relevance_id") {
        return metadataName(metadata.personalRelevanceLevels, value);
    }
    if (field == "lasting_impact_id") {
        return metadataName(metadata.lastingImpacts, value);
    }
    if (value.isNull()) {
        return "—";
    }
    if (field == "due_at") {
        return QDateTime::fromString(value.toString(), Qt::ISODate)
            .toLocalTime().toString("dd.MM.yyyy HH:mm");
    }
    if (value.isString()) {
        return value.toString();
    }
    if (value.isBool()) {
        return value.toBool() ? "Ja" : "Nein";
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble());
    }
    return {};
}

void showVersionHistoryDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const int entityId,
    const QString &title,
    const bool idea
) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.resize(820, 480);

    auto *layout = new QVBoxLayout(&dialog);
    auto *tree = new QTreeWidget(&dialog);
    tree->setColumnCount(5);
    tree->setHeaderLabels({"Zeitpunkt", "Version", "Feld", "Alt", "Neu"});
    tree->setRootIsDecorated(false);
    tree->setAlternatingRowColors(true);
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(3, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(4, QHeaderView::Stretch);
    layout->addWidget(tree);

    auto *loadingItem = new QTreeWidgetItem(tree);
    loadingItem->setText(0, "Versionshistorie wird geladen …");
    loadingItem->setFirstColumnSpanned(true);

    QObject::connect(
        &apiClient,
        &ApiClient::entityFieldVersionsReceived,
        &dialog,
        [&apiClient, entityId, tree, idea](
            const int receivedEntityId,
            const std::vector<EntityFieldVersion> &versions
        ) {
            if (receivedEntityId != entityId) {
                return;
            }
            tree->clear();
            if (versions.empty()) {
                auto *emptyItem = new QTreeWidgetItem(tree);
                emptyItem->setText(0, "Keine Änderungen vorhanden.");
                emptyItem->setFirstColumnSpanned(true);
                return;
            }
            for (const EntityFieldVersion &version : versions) {
                auto *item = new QTreeWidgetItem(tree);
                item->setText(
                    0,
                    version.changedAt.toLocalTime().toString("dd.MM.yyyy HH:mm:ss")
                );
                item->setText(1, QString::number(version.version));
                item->setText(2, fieldName(version.fieldName, idea));
                item->setText(
                    3,
                    fieldValue(
                        apiClient.metadata(), version.fieldName, version.oldValue, idea
                    )
                );
                item->setText(
                    4,
                    fieldValue(
                        apiClient.metadata(), version.fieldName, version.newValue, idea
                    )
                );
            }
        }
    );
    QObject::connect(
        &apiClient,
        &ApiClient::entityFieldVersionsLoadFailed,
        &dialog,
        [entityId, tree](const int failedEntityId) {
            if (failedEntityId != entityId) {
                return;
            }
            tree->clear();
            auto *errorItem = new QTreeWidgetItem(tree);
            errorItem->setText(0, "Versionshistorie konnte nicht geladen werden.");
            errorItem->setFirstColumnSpanned(true);
        }
    );

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    buttons->button(QDialogButtonBox::Close)->setText("Schließen");
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    prepareDialog(dialog, 820);

    apiClient.getEntityFieldVersions(entityId);
    execDialog(dialog);
}

void showDocumentVersionHistoryDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const int nodeId,
    const QString &title
) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.resize(820, 600);

    auto *layout = new QVBoxLayout(&dialog);
    auto *tree = new QTreeWidget(&dialog);
    tree->setColumnCount(2);
    tree->setHeaderLabels({"Zeitpunkt", "Version"});
    tree->setRootIsDecorated(false);
    tree->setAlternatingRowColors(true);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    layout->addWidget(tree);

    auto *preview = new QPlainTextEdit(&dialog);
    preview->setReadOnly(true);
    preview->setPlaceholderText("Version auswählen, um den Inhalt anzuzeigen.");
    layout->addWidget(preview, 1);

    auto *loadingItem = new QTreeWidgetItem(tree);
    loadingItem->setText(0, "Aufschrieb-Versionshistorie wird geladen …");
    loadingItem->setFirstColumnSpanned(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    auto *restoreButton = buttons->addButton(
        "Wiederherstellen",
        QDialogButtonBox::ActionRole
    );
    restoreButton->setEnabled(false);
    buttons->button(QDialogButtonBox::Close)->setText("Schließen");
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    std::vector<DocumentVersion> versions;
    QObject::connect(
        &apiClient,
        &ApiClient::documentVersionsReceived,
        &dialog,
        [nodeId, tree, preview, restoreButton, &versions](
            const int receivedNodeId,
            const std::vector<DocumentVersion> &receivedVersions
        ) {
            if (receivedNodeId != nodeId) {
                return;
            }
            versions = receivedVersions;
            tree->clear();
            preview->clear();
            restoreButton->setEnabled(false);
            if (versions.empty()) {
                auto *emptyItem = new QTreeWidgetItem(tree);
                emptyItem->setText(0, "Noch keine Aufschrieb-Versionen vorhanden.");
                emptyItem->setFirstColumnSpanned(true);
                return;
            }
            for (int index = 0; index < static_cast<int>(versions.size()); ++index) {
                const DocumentVersion &version = versions[index];
                auto *item = new QTreeWidgetItem(tree);
                item->setText(
                    0,
                    version.capturedAt.toLocalTime().toString("dd.MM.yyyy HH:mm:ss")
                );
                item->setText(1, QString::number(version.version));
                item->setData(0, Qt::UserRole, index);
            }
        }
    );
    QObject::connect(
        &apiClient,
        &ApiClient::documentVersionsLoadFailed,
        &dialog,
        [nodeId, tree](const int failedNodeId) {
            if (failedNodeId != nodeId) {
                return;
            }
            tree->clear();
            auto *errorItem = new QTreeWidgetItem(tree);
            errorItem->setText(0, "Aufschrieb-Versionshistorie konnte nicht geladen werden.");
            errorItem->setFirstColumnSpanned(true);
        }
    );
    QObject::connect(
        tree,
        &QTreeWidget::currentItemChanged,
        &dialog,
        [preview, restoreButton, &versions](QTreeWidgetItem *item) {
            if (!item || !item->data(0, Qt::UserRole).isValid()) {
                preview->clear();
                restoreButton->setEnabled(false);
                return;
            }
            preview->setPlainText(versions[item->data(0, Qt::UserRole).toInt()].content);
            restoreButton->setEnabled(true);
        }
    );
    QObject::connect(
        restoreButton,
        &QPushButton::clicked,
        &dialog,
        [&dialog, &apiClient, nodeId, tree, restoreButton] {
            QTreeWidgetItem *item = tree->currentItem();
            if (!item || QMessageBox::question(
                    &dialog,
                    "Aufschrieb-Version wiederherstellen",
                    "Die ausgewählte Version als aktuellen Inhalt des Aufschriebs wiederherstellen?"
                ) != QMessageBox::Yes) {
                return;
            }
            restoreButton->setEnabled(false);
            apiClient.restoreDocumentVersion(nodeId, item->text(1).toInt());
        }
    );
    QObject::connect(
        &apiClient,
        &ApiClient::documentVersionRestored,
        &dialog,
        [nodeId, &dialog](const Document &document) {
            if (document.domainNodeId == nodeId) {
                dialog.accept();
            }
        }
    );
    QObject::connect(
        &apiClient,
        &ApiClient::documentVersionRestoreFailed,
        &dialog,
        [nodeId, restoreButton](const int failedNodeId) {
            if (failedNodeId == nodeId) {
                restoreButton->setEnabled(true);
            }
        }
    );

    prepareDialog(dialog, 820);
    apiClient.getDocumentVersions(nodeId);
    execDialog(dialog);
}
