#include "ContextMenuStyle.h"

#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleOptionMenuItem>

namespace {

constexpr int menuWidth = 300;
constexpr int frameWidth = 1;
constexpr int verticalPadding = 4;
constexpr int rowHeight = 36;
constexpr int separatorHeight = 1;
constexpr int separatorPadding = 4;
constexpr int iconSize = 16;
constexpr int sidePadding = 16 - frameWidth;
constexpr int iconGap = 12;

const QColor background("#1C2633");
const QColor frame("#53647A");
const QColor separator("#45576D");
const QColor highlight("#315984");
const QColor textColor("#F0F4F9");
const QColor secondaryColor("#BED0E3");
const QColor disabledColor("#8999AB");
const QColor deleteIconColor("#FF9F99");

bool isActionMenu(const QWidget *widget) {
    return qobject_cast<const QMenu *>(widget) && widget->objectName() != "learningMetadataPopup";
}

class ContextMenuStyle final : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;

    void polish(QWidget *widget) override {
        QProxyStyle::polish(widget);
        if (isActionMenu(widget)) {
            widget->setWindowFlag(Qt::FramelessWindowHint);
            QFont font(QStringLiteral("Bahnschrift"));
            font.setPixelSize(14);
            widget->setFont(font);
            widget->setMinimumWidth(menuWidth);
        }
    }

    int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override {
        if (isActionMenu(widget)) {
            switch (metric) {
                case PM_MenuPanelWidth: return frameWidth;
                case PM_MenuHMargin: return 0;
                case PM_MenuVMargin: return verticalPadding;
                case PM_SmallIconSize: return iconSize;
                default: break;
            }
        }
        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                  QStyleHintReturn *returnData) const override {
        if (isActionMenu(widget)) {
            if (hint == SH_Menu_Scrollable) {
                return true;
            }
            if (hint == SH_Menu_Mask) {
                return false;
            }
        }
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }

    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size,
                           const QWidget *widget) const override {
        if (isActionMenu(widget)) {
            if (type == CT_Menu) {
                return size;
            }
            if (type == CT_MenuItem) {
                const auto *item = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
                const int height = item->menuItemType == QStyleOptionMenuItem::Separator
                    ? 2 * separatorPadding + separatorHeight : rowHeight;
                // QMenu adds the shortcut column and expands the rows to the menu's minimum width.
                return {1, height};
            }
        }
        return QProxyStyle::sizeFromContents(type, option, size, widget);
    }

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget) const override {
        if (isActionMenu(widget)) {
            if (element == PE_PanelMenu) {
                painter->fillRect(option->rect, background);
                return;
            }
            if (element == PE_FrameMenu) {
                painter->save();
                painter->setPen(frame);
                painter->setBrush(Qt::NoBrush);
                painter->drawRect(option->rect.adjusted(0, 0, -frameWidth, -frameWidth));
                painter->restore();
                return;
            }
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget) const override {
        if ((element != CE_MenuItem && element != CE_MenuScroller) || !isActionMenu(widget)) {
            QProxyStyle::drawControl(element, option, painter, widget);
            return;
        }

        const auto &item = *qstyleoption_cast<const QStyleOptionMenuItem *>(option);
        const QRect rect = item.rect;
        if (item.menuItemType == QStyleOptionMenuItem::Separator) {
            painter->fillRect(rect.x(), rect.y() + separatorPadding, rect.width(), separatorHeight, separator);
            return;
        }

        const bool enabled = item.state.testFlag(State_Enabled);
        const bool selected = enabled && item.state.testFlag(State_Selected);
        painter->fillRect(rect, selected ? highlight : background);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(enabled ? secondaryColor : disabledColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        if (element == CE_MenuScroller) {
            const QPoint center = rect.center();
            const int direction = item.state.testFlag(State_DownArrow) ? 1 : -1;
            painter->drawPolyline(QPolygonF{center + QPoint(-4, -2 * direction), center + QPoint(0, 2 * direction),
                                           center + QPoint(4, -2 * direction)});
            painter->restore();
            return;
        }

        const QRect iconRect(rect.x() + sidePadding, rect.y() + (rect.height() - iconSize) / 2, iconSize, iconSize);
        if (item.checked) {
            const QPoint origin = iconRect.topLeft();
            painter->drawPolyline(QPolygonF{origin + QPoint(2, 8), origin + QPoint(6, 12), origin + QPoint(14, 4)});
        } else if (!item.icon.isNull()) {
            item.icon.paint(painter, iconRect, Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
        }

        QRect labelRect = rect.adjusted(sidePadding + iconSize + iconGap, 0, -sidePadding, 0);
        if (item.menuItemType == QStyleOptionMenuItem::SubMenu) {
            const QPoint tip(labelRect.right() - 4, rect.center().y());
            painter->drawPolyline(QPolygonF{tip + QPoint(-4, -4), tip, tip + QPoint(-4, 4)});
            labelRect.adjust(0, 0, -iconSize, 0);
        }

        painter->setFont(item.font);
        const int textFlags = Qt::AlignVCenter | Qt::TextSingleLine;
        const QStringView text(item.text);
        const qsizetype tab = text.indexOf(u'\t');
        if (tab >= 0) {
            painter->setPen(enabled ? secondaryColor : disabledColor);
            painter->drawText(labelRect, textFlags | Qt::AlignRight, text.sliced(tab + 1).toString());
        }
        if (item.reservedShortcutWidth > 0) {
            labelRect.adjust(0, 0, -item.reservedShortcutWidth - iconGap, 0);
        }

        int mnemonic = Qt::TextShowMnemonic;
        if (!styleHint(SH_UnderlineShortcut, &item, widget, nullptr)) {
            mnemonic |= Qt::TextHideMnemonic;
        }
        const QString label = (tab < 0 ? text : text.first(tab)).toString();
        painter->setPen(enabled ? textColor : disabledColor);
        painter->drawText(labelRect, textFlags | Qt::AlignLeft | mnemonic,
                          item.fontMetrics.elidedText(label, Qt::ElideRight, labelRect.width(), mnemonic));
        painter->restore();
    }
};

} // namespace

void installContextMenuStyle() {
    QApplication::setStyle(new ContextMenuStyle(QApplication::style()->name()));
}

QIcon contextMenuDeleteIcon() {
    QPixmap icon(":/assets/icons/delete.png");
    QPainter painter(&icon);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(icon.rect(), deleteIconColor);
    painter.end();
    return QIcon(icon);
}
