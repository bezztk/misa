#include "DomainView.h"
#include "../models/AppState.h"

#include <QFrame>
#include <QLabel>
#include <QResizeEvent>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>

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
}

void DomainView::renderDomains() {
    domainCards.clear();

    while (QLayoutItem *item = cardsLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    for (const MetadataItem &domain : AppState::metadata.domains) {
        auto *card = new QFrame(this);
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
        QFont titleFont = title->font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 2);
        title->setFont(titleFont);
        cardLayout->addWidget(title);

        cardsLayout->addWidget(card, 0, Qt::AlignHCenter);
        domainCards.push_back(card);
    }

    updateCardWidths();
}

void DomainView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateCardWidths();
}

void DomainView::updateCardWidths() {
    const int cardWidth = std::min(1000, scrollArea->viewport()->width() - 40);

    for (QFrame *card : domainCards) {
        card->setFixedWidth(cardWidth);
    }
}
