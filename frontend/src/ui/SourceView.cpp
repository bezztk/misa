#include "SourceView.h"

#include "DialogUtils.h"
#include "../api/ApiClient.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr int ALL_CATEGORIES = -1;
constexpr int NO_CATEGORY = 0;
constexpr int ALL_TYPES = 0;
}

SourceView::SourceView(ApiClient &apiClient, QWidget *parent)
    : QWidget(parent), apiClient(apiClient) {
    auto *layout = new QVBoxLayout(this);

    auto *toolbar = new QHBoxLayout;
    search = new QLineEdit(this);
    search->setPlaceholderText("Quellen suchen");
    typeFilter = new QComboBox(this);
    categoryFilter = new QComboBox(this);
    toolbar->addWidget(search, 1);
    toolbar->addWidget(typeFilter);
    toolbar->addWidget(categoryFilter);
    layout->addLayout(toolbar);

    tree = new QTreeWidget(this);
    tree->setHeaderLabels({"Titel", "Typ", "Domain", "Beschreibung"});
    tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    auto *header = tree->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    layout->addWidget(tree);

    connect(search, &QLineEdit::textChanged, this, &SourceView::applyFilters);
    connect(typeFilter, &QComboBox::currentIndexChanged, this, &SourceView::applyFilters);
    connect(categoryFilter, &QComboBox::currentIndexChanged, this, &SourceView::applyFilters);
    connect(tree, &QTreeWidget::customContextMenuRequested,
            this, &SourceView::showContextMenu);
    connect(tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item) {
        if (item->parent()) {
            const Source &source = sources[item->data(0, Qt::UserRole).toInt()];
            QDesktopServices::openUrl(QUrl(source.url));
        }
    });

    connect(&apiClient, &ApiClient::sourcesReceived, this,
            [this](const std::vector<Source> &received) {
                sources = received;
                rebuildTree();
            });
    connect(&apiClient, &ApiClient::sourceCategoriesReceived, this,
            [this](const std::vector<SourceCategory> &received) {
                categories = received;
                rebuildTree();
            });
    connect(&apiClient, &ApiClient::sourcesChanged, this, [this] {
        this->apiClient.getSources();
    });
    connect(&apiClient, &ApiClient::sourceCategoriesChanged, this, &SourceView::refresh);
    rebuildTree();
    refresh();
}

void SourceView::refresh() {
    apiClient.getSourceCategories();
    apiClient.getSources();
}

void SourceView::openCreateDialog() {
    const auto url = showTextInputDialog(this, "Quelle hinzufügen", "URL", "Hinzufügen");
    if (url) {
        apiClient.createSource(*url);
    }
}

void SourceView::openCreateCategoryDialog() {
    if (const auto name = showTextInputDialog(this, "Kategorie erstellen", "Name")) {
        apiClient.createSourceCategory(*name);
    }
}

void SourceView::rebuildTree() {
    const QSignalBlocker blockTypeFilter(typeFilter);
    const QSignalBlocker blockCategoryFilter(categoryFilter);
    const QVariant selectedType = typeFilter->currentData();
    const QVariant selectedCategory = categoryFilter->currentData();
    typeFilter->clear();
    typeFilter->addItem("Alle Ressourcentypen", ALL_TYPES);
    QMap<QString, int> resourceTypes;
    for (const Source &source : sources) {
        resourceTypes.insert(source.resourceType, source.resourceTypeId);
    }
    for (auto type = resourceTypes.cbegin(); type != resourceTypes.cend(); ++type) {
        typeFilter->addItem(type.key(), type.value());
    }
    const int typeIndex = typeFilter->findData(selectedType);
    typeFilter->setCurrentIndex(qMax(0, typeIndex));

    categoryFilter->clear();
    categoryFilter->addItem("Alle Kategorien", ALL_CATEGORIES);
    categoryFilter->addItem("Ohne Kategorie", NO_CATEGORY);
    for (const SourceCategory &category : categories) {
        categoryFilter->addItem(category.name, category.id);
    }
    const int categoryIndex = categoryFilter->findData(selectedCategory);
    categoryFilter->setCurrentIndex(qMax(0, categoryIndex));

    QMap<int, bool> expandedGroups;
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        const auto *group = tree->topLevelItem(i);
        expandedGroups.insert(group->data(0, Qt::UserRole).toInt(), group->isExpanded());
    }
    tree->clear();

    QMap<int, QTreeWidgetItem *> groups;
    const auto addGroup = [&](const int id, const QString &name) {
        auto *group = new QTreeWidgetItem(tree, {name});
        group->setData(0, Qt::UserRole, id);
        group->setFirstColumnSpanned(true);
        auto font = group->font(0);
        font.setBold(true);
        group->setFont(0, font);
        groups.insert(id, group);
    };
    for (const SourceCategory &category : categories) {
        addGroup(category.id, category.name);
    }
    addGroup(NO_CATEGORY, "Ohne Kategorie");

    const QIcon standardIcon(":/assets/icons/file.png");
    for (std::size_t i = 0; i < sources.size(); ++i) {
        const Source &source = sources[i];
        auto *group = groups.value(source.categoryId);
        if (!group) {
            continue;
        }
        auto *item = new QTreeWidgetItem(group, {
            source.title, source.resourceType, QUrl(source.url).host(), source.description
        });
        item->setData(0, Qt::UserRole, static_cast<int>(i));
        item->setToolTip(2, source.url);
        QPixmap favicon;
        const bool hasFavicon = !source.favicon.isEmpty() && favicon.loadFromData(source.favicon);
        item->setIcon(0, hasFavicon ? QIcon(favicon) : standardIcon);
    }
    for (auto group = groups.cbegin(); group != groups.cend(); ++group) {
        group.value()->setExpanded(expandedGroups.value(group.key(), true));
    }
    applyFilters();
}

void SourceView::applyFilters() {
    const int categoryId = categoryFilter->currentData().toInt();
    const int typeId = typeFilter->currentData().toInt();
    const QString query = search->text().trimmed();
    const bool filterSources = !query.isEmpty() || typeId != ALL_TYPES;
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        auto *group = tree->topLevelItem(i);
        const int groupId = group->data(0, Qt::UserRole).toInt();
        bool hasMatch = false;
        for (int row = 0; row < group->childCount(); ++row) {
            auto *item = group->child(row);
            const Source &source = sources[item->data(0, Qt::UserRole).toInt()];
            const bool typeMatches = typeId == ALL_TYPES || source.resourceTypeId == typeId;
            const bool searchMatches = query.isEmpty()
                || source.title.contains(query, Qt::CaseInsensitive)
                || source.url.contains(query, Qt::CaseInsensitive)
                || source.description.contains(query, Qt::CaseInsensitive);
            const bool visible = typeMatches && searchMatches;
            item->setHidden(!visible);
            hasMatch |= visible;
        }
        const bool categoryMatches = categoryId == ALL_CATEGORIES || groupId == categoryId;
        const bool showEmpty = !filterSources && (groupId != NO_CATEGORY || categoryId == NO_CATEGORY);
        group->setHidden(!categoryMatches || (!hasMatch && !showEmpty));
    }
}

void SourceView::showContextMenu(const QPoint &position) {
    const auto *item = tree->itemAt(position);
    if (!item) {
        return;
    }
    const QPoint globalPosition = tree->viewport()->mapToGlobal(position);
    const int index = item->data(0, Qt::UserRole).toInt();
    if (item->parent()) {
        const Source source = sources[index];
        showSourceMenu(source, globalPosition);
    } else if (index != NO_CATEGORY) {
        const QString name = item->text(0);
        showCategoryMenu(index, name, globalPosition);
    }
}

void SourceView::showSourceMenu(const Source &source, const QPoint &position) {
    QMenu menu(this);
    const auto *open = menu.addAction("Quelle öffnen");
    const auto *copy = menu.addAction("URL kopieren");
    const auto *rename = menu.addAction("Titel ändern");
    const auto *assign = menu.addAction("Kategorie ändern");
    menu.addSeparator();
    const auto *remove = menu.addAction("Quelle löschen");
    const auto *selected = menu.exec(position);
    if (selected == open) {
        QDesktopServices::openUrl(QUrl(source.url));
    } else if (selected == copy) {
        QApplication::clipboard()->setText(source.url);
    } else if (selected == rename) {
        if (const auto title = showTextInputDialog(
                this, "Titel ändern", "Titel", "Speichern", source.title)) {
            apiClient.updateSourceTitle(source.id, *title);
        }
    } else if (selected == assign) {
        openCategoryDialog(source);
    } else if (selected == remove && QMessageBox::question(
                   this, "Quelle löschen", "Diese Quelle löschen?",
                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        apiClient.deleteSource(source.id);
    }
}

void SourceView::showCategoryMenu(const int id, const QString &name, const QPoint &position) {
    QMenu menu(this);
    const auto *rename = menu.addAction("Kategorie umbenennen");
    const auto *remove = menu.addAction("Kategorie löschen");
    const auto *selected = menu.exec(position);
    if (selected == rename) {
        if (const auto newName = showTextInputDialog(
                this, "Kategorie umbenennen", "Name", "Speichern", name)) {
            apiClient.renameSourceCategory(id, *newName);
        }
    } else if (selected == remove && QMessageBox::question(
                   this, "Kategorie löschen",
                   "Diese Kategorie löschen? Ihre Quellen erscheinen danach unter „Ohne Kategorie“.",
                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        apiClient.deleteSourceCategory(id);
    }
}

void SourceView::openCategoryDialog(const Source &source) {
    QDialog dialog(this);
    dialog.setWindowTitle("Kategorie ändern");
    prepareDialog(dialog);
    auto *layout = new QFormLayout(&dialog);
    auto *choices = new QComboBox(&dialog);
    choices->addItem("Ohne Kategorie", NO_CATEGORY);
    for (const SourceCategory &category : categories) {
        choices->addItem(category.name, category.id);
    }
    const int categoryIndex = choices->findData(source.categoryId);
    choices->setCurrentIndex(qMax(0, categoryIndex));
    layout->addRow("Kategorie", choices);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText("Speichern");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (execDialog(dialog) == QDialog::Accepted) {
        apiClient.assignSourceCategory(source.id, choices->currentData().toInt());
    }
}
