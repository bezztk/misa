#ifndef FRONTEND_NAVIGATIONLISTDELEGATE_H
#define FRONTEND_NAVIGATIONLISTDELEGATE_H

#include <QIcon>
#include <QPainter>
#include <QStyledItemDelegate>

class NavigationListDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    static constexpr int CountRole = Qt::UserRole + 1;

    void paint(
        QPainter *painter,
        const QStyleOptionViewItem &option,
        const QModelIndex &index
    ) const override {
        const QRect row(0, option.rect.top(), painter->viewport().width(), option.rect.height());
        painter->save();
        if (!index.flags().testFlag(Qt::ItemIsEnabled)) {
            QColor divider = option.palette.text().color();
            divider.setAlpha(38);
            painter->setPen(divider);
            painter->drawLine(row.left() + 18, row.center().y(), row.right() - 18, row.center().y());
            painter->restore();
            return;
        }
        painter->fillRect(row, index.data(Qt::BackgroundRole).value<QBrush>());
        if (option.state.testFlag(QStyle::State_MouseOver)) {
            painter->fillRect(row, QColor(255, 255, 255, 26));
        }
        if (option.state.testFlag(QStyle::State_Selected)) {
            painter->fillRect(QRect(row.topLeft(), QSize(3, row.height())), QColor("#B8B8B8"));
        }

        QRect textRect = row.adjusted(18, 0, -18, 0);
        constexpr int iconSize = 22;
        const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        if (!icon.isNull()) {
            const QRect iconRect(
                textRect.left(), row.top() + (row.height() - iconSize) / 2,
                iconSize, iconSize
            );
            icon.paint(painter, iconRect);
            textRect.adjust(iconSize + 12, 0, 0, 0);
        }

        const QString count = index.data(CountRole).toString();
        auto countFont = option.font;
        countFont.setPixelSize(12);
        const int countWidth = QFontMetrics(countFont).horizontalAdvance(count);
        const QRect countRect(textRect.right() + 1 - countWidth, row.top(), countWidth, row.height());
        QColor countColor = option.palette.text().color();
        countColor.setAlpha(175);
        painter->setFont(countFont);
        painter->setPen(countColor);
        painter->drawText(countRect, Qt::AlignRight | Qt::AlignVCenter, count);

        textRect.setRight(countRect.left() - 12);
        painter->setFont(option.font);
        painter->setPen(option.palette.text().color());
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
            option.fontMetrics.elidedText(index.data().toString(), Qt::ElideRight, textRect.width()));
        painter->restore();
    }
};

#endif // FRONTEND_NAVIGATIONLISTDELEGATE_H
