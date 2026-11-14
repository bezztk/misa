#include "DomainView.h"

#include <QFrame>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QTreeView>
#include <QVBoxLayout>

#include <functional>

static constexpr int DOMAIN_ID_ROLE = Qt::UserRole + 1;

DomainView::DomainView(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *splitter = new QSplitter(Qt::Horizontal, this);

    treeView = new QTreeView(splitter);
    treeView->setHeaderHidden(true);
    treeView->setFrameShape(QFrame::NoFrame);
    treeView->setIndentation(16);
    treeView->setUniformRowHeights(true);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    model = new QStandardItemModel(treeView);
    treeView->setModel(model);

    splitter->addWidget(treeView);
    splitter->addWidget(new QTextEdit(splitter));
    layout->addWidget(splitter);

    connect(&apiClient, &ApiClient::domainTreeReceived, this,
            [this](const std::vector<MetadataItem> &domains,
                   const std::vector<DomainNode> &nodes) {
                renderDomainTree(domains, nodes);
            });
    connect(&apiClient, &ApiClient::domainNodeCreated, this,
            [this](const DomainNode &node) {
                auto *item = new QStandardItem(node.name);
                item->setData(node.id, Qt::UserRole);
                QStandardItem *parentItem = node.parentId.has_value()
                    ? findDomainNodeItem(*node.parentId)
                    : findDomainItem(node.domainId);
                parentItem->appendRow(item);
            });
    connect(&apiClient, &ApiClient::domainNodeUpdated, this,
            [this](const DomainNode &node) {
                findDomainNodeItem(node.id)->setText(node.name);
            });
    connect(treeView, &QTreeView::customContextMenuRequested, this,
            [this](const QPoint &position) {
                showDomainNodeContextMenu(position);
            });

    apiClient.getDomainTree();
}

void DomainView::renderDomainTree(
    const std::vector<MetadataItem> &domains,
    const std::vector<DomainNode> &nodes
) {
    model->clear();

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

                auto *item = new QStandardItem(node.name);
                item->setData(node.id, Qt::UserRole);
                parentItem->appendRow(item);
                addNodes(node.id, item);
            }
        };

        addNodes(std::nullopt, domainItem);
    }

    treeView->collapseAll();
}

void DomainView::showDomainNodeContextMenu(const QPoint &position) {
    const QModelIndex index = treeView->indexAt(position);

    if (!index.isValid()) {
        return;
    }

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
    QAction *selectedAction = menu.exec(treeView->viewport()->mapToGlobal(position));

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
            apiClient.createDomainNode(index.data(Qt::UserRole).toInt(), name);
        }
    } else if (selectedAction == editAction) {
        bool accepted;
        const QString name = QInputDialog::getText(
            treeView,
            "Domain Node bearbeiten",
            "Name",
            QLineEdit::Normal,
            index.data().toString(),
            &accepted
        ).trimmed();

        if (accepted && !name.isEmpty()) {
            apiClient.updateDomainNode(index.data(Qt::UserRole).toInt(), name);
        }
    }
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
