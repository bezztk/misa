#include "DomainView.h"
#include "../models/AppState.h"

#include <QFrame>
#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

DomainView::DomainView(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *cards = new QWidget(scrollArea);
    cardsLayout = new QVBoxLayout(cards);
    cardsLayout->setContentsMargins(20, 20, 20, 20);
    cardsLayout->setSpacing(12);
    cardsLayout->setAlignment(Qt::AlignTop);

    scrollArea->setWidget(cards);
    layout->addWidget(scrollArea);

    connect(&apiClient, &ApiClient::domainNodesReceived, this,
            [this](const int domainId, const std::vector<DomainNode> &nodes) {
                renderDomainNodes(domainId, nodes);
            });
}

void DomainView::renderDomains() {
    domainCards.clear();

    while (QLayoutItem *item = cardsLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    for (const MetadataItem &domain : AppState::metadata.domains) {
        auto *card = new QFrame(this);
        card->setCursor(Qt::PointingHandCursor);
        card->installEventFilter(this);
        card->setMaximumWidth(1000);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        card->setStyleSheet(R"(
            QFrame {
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

        cardsLayout->addWidget(card, 0, Qt::AlignHCenter);
        domainCards.insert(domain.id, DomainCard{
            .domain = domain,
            .frame = card,
            .isLoaded = false
        });
    }

    updateCardWidths();
}

bool DomainView::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonRelease
        && static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton) {
        for (auto it = domainCards.begin(); it != domainCards.end(); ++it) {
            DomainCard &card = it.value();

            if (card.frame == watched && !card.isLoaded) {
                card.isLoaded = true;
                apiClient.getDomainNodes(card.domain.id);
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
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(domainCards[domainId].frame->layout());

    std::function<void(std::optional<int>, int)> addNodes;
    addNodes = [&](const std::optional<int> parentId, const int depth) {
        for (const DomainNode &node : nodes) {
            if (node.parentId != parentId) {
                continue;
            }

            auto *label = new QLabel(node.name, domainCards[domainId].frame);
            label->setContentsMargins(depth * 18, 0, 0, 0);
            label->setAttribute(Qt::WA_TransparentForMouseEvents);
            layout->addWidget(label);
            addNodes(node.id, depth + 1);
        }
    };

    addNodes(std::nullopt, 0);
}

void DomainView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateCardWidths();
}

void DomainView::updateCardWidths() {
    const int cardWidth = std::min(1000, scrollArea->viewport()->width() - 40);

    for (const DomainCard &card : domainCards) {
        card.frame->setFixedWidth(cardWidth);
    }
}
