#include "DomainView.h"
#include "../models/AppState.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollArea>
#include <QShowEvent>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

DomainView::DomainView(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    cardsWidget = new QWidget(scrollArea);
    cardsLayout = new QHBoxLayout(cardsWidget);
    cardsLayout->setContentsMargins(20, 20, 20, 20);
    cardsLayout->setSpacing(12);
    cardsLayout->setAlignment(Qt::AlignTop);

    auto *firstColumn = new QWidget(cardsWidget);
    firstColumnLayout = new QVBoxLayout(firstColumn);
    firstColumnLayout->setContentsMargins(0, 0, 0, 0);
    firstColumnLayout->setSpacing(12);
    firstColumnLayout->setAlignment(Qt::AlignTop);

    secondColumn = new QWidget(cardsWidget);
    secondColumnLayout = new QVBoxLayout(secondColumn);
    secondColumnLayout->setContentsMargins(0, 0, 0, 0);
    secondColumnLayout->setSpacing(12);
    secondColumnLayout->setAlignment(Qt::AlignTop);

    cardsLayout->addWidget(firstColumn);
    cardsLayout->addWidget(secondColumn);

    scrollArea->setWidget(cardsWidget);
    layout->addWidget(scrollArea);

    connect(&apiClient, &ApiClient::domainNodesReceived, this,
            [this](const std::vector<DomainNode> &nodes) {
                for (const MetadataItem &domain : AppState::metadata.domains) {
                    renderDomainNodes(domain.id, nodes);
                }
            });
    connect(&apiClient, &ApiClient::domainNodeCreated, this,
            [this](const int parentId, const DomainNode &node) {
                QStandardItem *parentItem = findDomainNodeItem(node.domainId, parentId);
                auto *item = new QStandardItem(node.name);
                item->setData(node.id, Qt::UserRole);
                parentItem->appendRow(item);
                updateTreeHeight(domainCards[node.domainId].treeView);
            });
    connect(&apiClient, &ApiClient::domainNodeUpdated, this,
            [this](const DomainNode &node) {
                findDomainNodeItem(node.domainId, node.id)->setText(node.name);
            });
}

void DomainView::renderDomains() {
    domainCards.clear();

    for (QVBoxLayout *columnLayout : {firstColumnLayout, secondColumnLayout}) {
        while (QLayoutItem *item = columnLayout->takeAt(0)) {
            delete item->widget();
            delete item;
        }
    }

    for (const MetadataItem &domain : AppState::metadata.domains) {
        auto *card = new QFrame(this);
        card->setObjectName("domainCard");
        card->setCursor(Qt::PointingHandCursor);
        card->installEventFilter(this);
        card->setMaximumWidth(600);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        card->setStyleSheet(R"(
            QFrame#domainCard {
                background: rgba(255, 255, 255, 0.05);
                border: 1px solid rgba(255, 255, 255, 0.10);
                border-radius: 6px;
            }
            QLabel {
                border: none;
                background: transparent;
            }
        )");

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 14, 16, 14);

        auto *title = new QLabel(domain.name, card);
        title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        title->setAttribute(Qt::WA_TransparentForMouseEvents);
        QFont titleFont = title->font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 2);
        title->setFont(titleFont);
        cardLayout->addWidget(title);

        domainCards.insert(domain.id, DomainCard{
            .domain = domain,
            .frame = card,
            .treeView = nullptr,
            .model = nullptr,
            .isLoaded = false
        });
    }

    updateCardLayout();
    apiClient.getDomainNodes();
}

bool DomainView::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonRelease
        && static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton) {
        for (auto it = domainCards.begin(); it != domainCards.end(); ++it) {
            DomainCard &card = it.value();

            if (card.frame == watched && card.isLoaded) {
                card.treeView->setVisible(!card.treeView->isVisible());
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void DomainView::renderDomainNodes(
    const int domainId,
    const std::vector<DomainNode> &nodes
) {
    DomainCard &card = domainCards[domainId];
    auto *layout = qobject_cast<QVBoxLayout *>(card.frame->layout());

    card.treeView = new QTreeView(card.frame);
    card.model = new QStandardItemModel(card.treeView);
    card.treeView->setHeaderHidden(true);
    card.treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    card.treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    card.treeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    card.treeView->setModel(card.model);

    connect(card.treeView, &QTreeView::customContextMenuRequested, this,
            [this, domainId](const QPoint &position) {
                showDomainNodeContextMenu(domainId, position);
            });

    std::function<void(std::optional<int>, QStandardItem *)> addNodes;
    addNodes = [&](const std::optional<int> parentId, QStandardItem *parentItem) {
        for (const DomainNode &node : nodes) {
            if (node.domainId != domainId || node.parentId != parentId) {
                continue;
            }

            auto *item = new QStandardItem(node.name);
            item->setData(node.id, Qt::UserRole);
            parentItem->appendRow(item);
            addNodes(node.id, item);
        }
    };

    addNodes(std::nullopt, card.model->invisibleRootItem());
    layout->addWidget(card.treeView);
    card.treeView->hide();
    card.isLoaded = true;

    connect(card.treeView, &QTreeView::expanded, this, [this, treeView = card.treeView] {
        updateTreeHeight(treeView);
    });
    connect(card.treeView, &QTreeView::collapsed, this, [this, treeView = card.treeView] {
        updateTreeHeight(treeView);
    });

    updateTreeHeight(card.treeView);
}

void DomainView::showDomainNodeContextMenu(
    const int domainId,
    const QPoint &position
) {
    DomainCard &card = domainCards[domainId];
    const QModelIndex index = card.treeView->indexAt(position);

    if (!index.isValid()) {
        return;
    }

    QMenu menu(card.treeView);
    QAction *createAction = menu.addAction("Unterknoten hinzufügen");
    QAction *editAction = menu.addAction("Bearbeiten");
    QAction *selectedAction = menu.exec(card.treeView->viewport()->mapToGlobal(position));

    if (selectedAction == createAction) {
        bool accepted;
        const QString name = QInputDialog::getText(
            card.treeView,
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
            card.treeView,
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

QStandardItem *DomainView::findDomainNodeItem(const int domainId, const int nodeId) {
    QStandardItemModel *model = domainCards[domainId].model;
    const QModelIndexList matches = model->match(
        model->index(0, 0),
        Qt::UserRole,
        nodeId,
        1,
        Qt::MatchExactly | Qt::MatchRecursive
    );

    return model->itemFromIndex(matches.first());
}

void DomainView::updateTreeHeight(QTreeView *treeView) {
    int height = treeView->frameWidth() * 2;

    for (QModelIndex index = treeView->model()->index(0, 0);
         index.isValid();
         index = treeView->indexBelow(index)) {
        height += treeView->sizeHintForIndex(index).height();
    }

    treeView->setFixedHeight(std::min(height, 400));
}

void DomainView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateCardLayout();
}

void DomainView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    updateCardLayout();
}

void DomainView::updateCardLayout() {
    const int availableWidth = scrollArea->viewport()->width() - 40;
    const int columnCount = availableWidth >= 1212 ? 2 : 1;
    const int cardWidth = std::min(600, (availableWidth - (columnCount - 1) * 12) / columnCount);

    cardsWidget->setFixedWidth(cardWidth * columnCount + (columnCount - 1) * 12 + 40);
    secondColumn->setVisible(columnCount == 2);

    int index = 0;
    for (const MetadataItem &domain : AppState::metadata.domains) {
        if (!domainCards.contains(domain.id)) {
            continue;
        }

        const DomainCard &card = domainCards[domain.id];
        firstColumnLayout->removeWidget(card.frame);
        secondColumnLayout->removeWidget(card.frame);
        card.frame->setFixedWidth(cardWidth);
        QVBoxLayout *columnLayout = columnCount == 2 && index % 2 == 1
            ? secondColumnLayout
            : firstColumnLayout;
        columnLayout->addWidget(card.frame);
        ++index;
    }
}
