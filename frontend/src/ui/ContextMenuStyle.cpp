#include "ContextMenuStyle.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QMenu>
#include <QPainter>
#include <QProxyStyle>
#include <QStyledItemDelegate>
#include <QStyleOptionComboBox>
#include <QStyleOptionMenuItem>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <dwmapi.h>
#endif

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
constexpr int comboHeight = 38;
constexpr int comboInset = 12;
constexpr int comboArrowWidth = 32;
constexpr int comboPreferredWidthLimit = 320;

const QColor background("#242424");
const QColor frame("#5E5E5E");
const QColor separator("#505050");
const QColor highlight("#414141");
const QColor textColor("#F4F4F4");
const QColor secondaryColor("#CCCCCC");
const QColor disabledColor("#999999");
const QColor deleteIconColor("#FF9F99");
const QColor comboSurface("#2B2B2B");
const QColor comboSelection("#353535");
const QColor comboFocus("#B8B8B8");

bool isActionMenu(const QWidget *widget) {
    return qobject_cast<const QMenu *>(widget) && widget->objectName() != "learningMetadataPopup";
}

class ComboBoxItemDelegate final : public QStyledItemDelegate {
public:
    explicit ComboBoxItemDelegate(QComboBox &combo)
        : QStyledItemDelegate(combo.view()), combo(combo) {}

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        const QSize size = QStyledItemDelegate::sizeHint(option, index);
        return {size.width() + 2 * comboInset, qMax(comboHeight, size.height() + 12)};
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem item(option);
        initStyleOption(&item, index);
        const QRect viewport = combo.view()->viewport()->rect();
        item.rect.setLeft(viewport.left());
        item.rect.setRight(viewport.right());
        item.textElideMode = Qt::ElideRight;
        const bool enabled = item.state.testFlag(QStyle::State_Enabled);
        const bool active = enabled && item.state.testAnyFlags(QStyle::State_Selected | QStyle::State_MouseOver);
        const bool current = index.row() == combo.currentIndex();

        painter->fillRect(item.rect, active ? highlight : (current ? comboSelection : background));
        if (current) {
            const QRect marker(item.rect.topLeft(), QSize(2, item.rect.height()));
            painter->fillRect(QStyle::visualRect(item.direction, item.rect, marker), comboFocus);
        }

        item.rect.adjust(comboInset, 0, -comboInset, 0);
        item.state &= ~(QStyle::State_Selected | QStyle::State_MouseOver | QStyle::State_HasFocus);
        item.backgroundBrush = Qt::NoBrush;
        item.palette.setColor(QPalette::Text, enabled ? textColor : disabledColor);
        combo.view()->style()->drawControl(QStyle::CE_ItemViewItem, &item, painter, item.widget);
    }

private:
    const QComboBox &combo;
};

class WidgetStyle final : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;

    void polish(QWidget *widget) override {
        QProxyStyle::polish(widget);
        if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            QFont font(QStringLiteral("Bahnschrift"));
            font.setPixelSize(14);
            font.setWeight(QFont::Medium);
            combo->setFont(font);
            combo->setAttribute(Qt::WA_Hover);
            auto *view = combo->view();
            view->setFont(font);
            view->setTextElideMode(Qt::ElideRight);
            view->setStyleSheet("QAbstractItemView { background: #242424; border: 1px solid #5E5E5E; border-radius: 0; }");
#ifdef Q_OS_WIN
            view->window()->installEventFilter(this);
#endif
            if (!dynamic_cast<ComboBoxItemDelegate *>(combo->itemDelegate())) {
                combo->setItemDelegate(new ComboBoxItemDelegate(*combo));
            }
        } else if (isActionMenu(widget)) {
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
        if (qobject_cast<const QComboBox *>(widget)) {
            switch (hint) {
                case SH_ComboBox_Popup: return false;
                case SH_ComboBox_ListMouseTracking: return true;
                case SH_ComboBox_PopupFrameStyle: return QFrame::NoFrame;
                default: break;
            }
        }
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
        if (type == CT_ComboBox) {
            const int width = size.width() + 2 * comboInset + comboArrowWidth + 2;
            return {qMin(width, comboPreferredWidthLimit), qMax(comboHeight, size.height() + 14)};
        }
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

    QRect subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                         SubControl subControl, const QWidget *widget) const override {
        if (control == CC_ComboBox) {
            if (subControl == SC_ComboBoxListBoxPopup) {
                return option->rect;
            }
            const QRect bounds = option->rect.adjusted(1, 1, -1, -1);
            if (subControl == SC_ComboBoxArrow) {
                const QRect arrow(bounds.right() - comboArrowWidth + 1, bounds.top(), comboArrowWidth, bounds.height());
                return visualRect(option->direction, option->rect, arrow);
            }
            if (subControl == SC_ComboBoxEditField) {
                const QRect label = bounds.adjusted(comboInset - 1, 0, -comboArrowWidth - comboInset, 0);
                return visualRect(option->direction, option->rect, label);
            }
        }
        return QProxyStyle::subControlRect(control, option, subControl, widget);
    }

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const override {
        if (control != CC_ComboBox) {
            QProxyStyle::drawComplexControl(control, option, painter, widget);
            return;
        }

        const bool enabled = option->state.testFlag(State_Enabled);
        const bool open = option->state.testFlag(State_On);
        const bool focused = enabled && (open || option->state.testFlag(State_HasFocus));
        const bool hovered = enabled && option->state.testFlag(State_MouseOver);
        const QRect rect = option->rect;
        const QRect arrow = subControlRect(control, option, SC_ComboBoxArrow, widget);
        const int separatorX = option->direction == Qt::RightToLeft ? arrow.right() : arrow.left();

        painter->save();
        painter->fillRect(rect, hovered || open ? comboSelection : comboSurface);
        painter->setPen(focused ? comboFocus : frame);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect.adjusted(0, 0, -1, -1));
        painter->setPen(separator);
        painter->drawLine(separatorX, arrow.top(), separatorX, arrow.bottom());

        const QPoint center = arrow.center();
        const int direction = open ? -1 : 1;
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(enabled ? secondaryColor : disabledColor, 1.5, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
        painter->drawPolyline(QPolygonF{center + QPoint(-5, -2 * direction), center + QPoint(0, 3 * direction),
                                       center + QPoint(5, -2 * direction)});
        painter->restore();
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
        if (element == CE_ComboBoxLabel) {
            QStyleOptionComboBox label(*qstyleoption_cast<const QStyleOptionComboBox *>(option));
            const QRect field = subControlRect(CC_ComboBox, &label, SC_ComboBoxEditField, widget);
            const int iconWidth = label.currentIcon.isNull() ? 0 : label.iconSize.width() + 4;
            const int textWidth = qMax(0, field.width() - iconWidth - 4);
            label.currentText = label.fontMetrics.elidedText(label.currentText, Qt::ElideRight, textWidth);
            const auto color = label.state.testFlag(State_Enabled) ? textColor : disabledColor;
            label.palette.setColor(QPalette::Text, color);
            label.palette.setColor(QPalette::ButtonText, color);
            QProxyStyle::drawControl(element, &label, painter, widget);
            return;
        }
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

#ifdef Q_OS_WIN
protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::Show) {
            const auto *popup = static_cast<QWidget *>(watched);
            constexpr auto corners = DWMWCP_DONOTROUND;
            DwmSetWindowAttribute(reinterpret_cast<HWND>(popup->winId()), DWMWA_WINDOW_CORNER_PREFERENCE,
                                  &corners, sizeof(corners));
        }
        return QProxyStyle::eventFilter(watched, event);
    }
#endif
};

} // namespace

void installWidgetStyle() {
    QApplication::setStyle(new WidgetStyle(QApplication::style()->name()));
}

QIcon contextMenuDeleteIcon() {
    QPixmap icon(":/assets/icons/delete.png");
    QPainter painter(&icon);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(icon.rect(), deleteIconColor);
    painter.end();
    return QIcon(icon);
}
