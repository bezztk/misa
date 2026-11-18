#include "VersionHistoryDialog.h"

#include "../api/ApiClient.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QTreeWidget>
#include <QVBoxLayout>

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

static QString fieldName(const QString &name) {
    if (name == "name") return "Name";
    if (name == "cognitive_level_id") return "Kognitives Niveau";
    if (name == "completeness_level_id") return "Vollständigkeit";
    if (name == "flashcard_status_id") return "Lernkartenstatus";
    if (name == "verification_status_id") return "Verifizierungsstatus";
    if (name == "verification_level_id") return "Verifizierungsniveau";
    if (name == "state_id") return "Status";
    if (name == "priority_id") return "Priorität";
    if (name == "psychomotor_level_id") return "Psychomotorisches Niveau";
    if (name == "front") return "Vorderseite";
    if (name == "back") return "Rückseite";
    if (name == "personal_relevance_id") return "Persönliche Relevanz";
    if (name == "lasting_impact_id") return "Langfristige Auswirkung";
    return name;
}

static QString fieldValue(
    const Metadata &metadata,
    const QString &field,
    const QJsonValue &value
) {
    if (field == "cognitive_level_id") {
        return metadataName(metadata.cognitiveLevels, value);
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
        return metadataName(metadata.systemTaskStates, value);
    }
    if (field == "priority_id") {
        return metadataName(metadata.systemTaskPriorities, value);
    }
    if (field == "psychomotor_level_id") {
        return metadataName(metadata.psychomotorLevels, value);
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
    const QString &title
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
        [&apiClient, entityId, tree](
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
                item->setText(2, fieldName(version.fieldName));
                item->setText(
                    3,
                    fieldValue(apiClient.metadata(), version.fieldName, version.oldValue)
                );
                item->setText(
                    4,
                    fieldValue(apiClient.metadata(), version.fieldName, version.newValue)
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
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    apiClient.getEntityFieldVersions(entityId);
    dialog.exec();
}
