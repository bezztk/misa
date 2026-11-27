#include "ContextView.h"

#include "ContextMenuStyle.h"

#include "DialogUtils.h"
#include "../api/ApiClient.h"

#include <QBuffer>
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFontMetrics>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace {
constexpr QSize MAX_CARD_SIZE(536, 112);
constexpr int CARD_GAP = 12;
constexpr QSize CELL_GAP(CARD_GAP, CARD_GAP);
constexpr int CARD_MARGIN = 14;
constexpr int MENU_SIZE = 28;
constexpr int ICON_SIZE = 24;

class ContextCardTitle final : public QLabel {
public:
    ContextCardTitle(QString title, QWidget *parent) : QLabel(parent), title(std::move(title)) {
        setTextFormat(Qt::PlainText);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        auto titleFont = font();
        titleFont.setPixelSize(22);
        titleFont.setWeight(QFont::DemiBold);
        setFont(titleFont);
        setText(this->title);
        setToolTip(this->title);
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QLabel::resizeEvent(event);
        setText(fontMetrics().elidedText(title, Qt::ElideRight, contentsRect().width()));
    }

private:
    QString title;
};

QString cardBackground(const QColor &color) {
    if (!color.isValid()) {
        return "rgba(128, 128, 128, 18)";
    }
    const QColor tint = contextTint(color);
    return QString("rgba(%1, %2, %3, %4)")
        .arg(tint.red()).arg(tint.green()).arg(tint.blue()).arg(tint.alpha());
}
}

ContextView::ContextView(ApiClient &apiClient, QWidget *parent)
    : QWidget(parent), apiClient(apiClient), grid(new QListWidget(this)) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 14, 16, 8);

    grid->setAccessibleName("Kontexte");
    grid->setFrameShape(QFrame::NoFrame);
    grid->setViewMode(QListView::IconMode);
    grid->setFlow(QListView::LeftToRight);
    grid->setWrapping(true);
    grid->setResizeMode(QListView::Adjust);
    grid->setMovement(QListView::Static);
    grid->setGridSize(MAX_CARD_SIZE + CELL_GAP);
    grid->setSelectionMode(QAbstractItemView::NoSelection);
    grid->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    grid->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    grid->setStyleSheet("QListWidget { background: transparent; }");
    layout->addWidget(grid);

    connect(&apiClient, &ApiClient::contextsReceived, this, &ContextView::renderContexts);
    const auto refreshCounts = [this] { this->apiClient.getContexts(); };
    connect(&apiClient, &ApiClient::taskCreated, this, refreshCounts);
    connect(&apiClient, &ApiClient::taskUpdated, this, refreshCounts);
    connect(&apiClient, &ApiClient::taskDeleted, this, refreshCounts);
    connect(&apiClient, &ApiClient::ideaUpdated, this, refreshCounts);
    connect(&apiClient, &ApiClient::ideaDeleted, this, refreshCounts);
    apiClient.getContexts();
}

void ContextView::openCreateDialog() {
    openContextDialog(std::nullopt);
}

void ContextView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    const QMargins margins = layout()->contentsMargins();
    const int scrollBarWidth = grid->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    const int cardWidth = std::clamp(
        width() - margins.left() - margins.right() - scrollBarWidth - CARD_GAP,
        1, MAX_CARD_SIZE.width()
    );
    const QSize cardSize(cardWidth, MAX_CARD_SIZE.height());
    const QSize cellSize = cardSize + CELL_GAP;
    if (cellSize == grid->gridSize()) {
        return;
    }

    grid->setGridSize(cellSize);
    for (int row = 0; row < grid->count(); ++row) {
        auto *item = grid->item(row);
        item->setSizeHint(cardSize);
        grid->itemWidget(item)->setFixedWidth(cardWidth);
    }
}

void ContextView::renderContexts(const std::vector<Context> &contexts) {
    const int scrollPosition = grid->verticalScrollBar()->value();
    const QSize cardSize = grid->gridSize() - CELL_GAP;
    grid->clear();
    for (const Context &context : contexts) {
        const QString countText = context.entityCount == 1
            ? "1 verknüpfter Eintrag"
            : QString("%1 verknüpfte Einträge").arg(context.entityCount);
        auto *item = new QListWidgetItem(grid);
        item->setSizeHint(cardSize);
        item->setData(Qt::AccessibleTextRole, context.title);
        item->setData(Qt::AccessibleDescriptionRole, countText);
        item->setToolTip(context.title);

        auto *card = new QFrame(grid->viewport());
        card->setObjectName("contextCard");
        card->setFixedSize(cardSize);
        card->setAttribute(Qt::WA_StyledBackground);
        card->setStyleSheet(QString(R"(
            QFrame#contextCard {
                background-color: %1;
                border: 1px solid rgba(128, 128, 128, 70);
                border-radius: 8px;
            }
            QToolButton {
                background: transparent;
                border: 0;
                border-radius: 4px;
            }
            QToolButton:hover, QToolButton:focus {
                background: rgba(128, 128, 128, 45);
            }
            QToolButton::menu-indicator { image: none; }
        )").arg(cardBackground(context.color)));

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(CARD_MARGIN, 10, CARD_MARGIN, 12);
        cardLayout->setSpacing(3);

        auto *top = new QHBoxLayout;
        top->setSpacing(10);
        const QIcon icon = contextIcon(context.icon);
        if (!icon.isNull()) {
            auto *iconLabel = new QLabel(card);
            iconLabel->setPixmap(icon.pixmap(QSize(ICON_SIZE, ICON_SIZE), card->devicePixelRatioF()));
            top->addWidget(iconLabel);
        }
        top->addWidget(new ContextCardTitle(context.title, card), 1);
        auto *actions = new QToolButton(card);
        actions->setIcon(QIcon(":/assets/icons/option.png"));
        actions->setIconSize(QSize(18, 18));
        actions->setAccessibleName("Aktionen für " + context.title);
        actions->setToolTip("Kontextaktionen");
        actions->setFixedSize(MENU_SIZE, MENU_SIZE);
        actions->setPopupMode(QToolButton::InstantPopup);
        auto *menu = new QMenu(actions);
        auto *edit = menu->addAction(QIcon(":/assets/icons/edit.png"), "Bearbeiten");
        auto *remove = menu->addAction(contextMenuDeleteIcon(), "Löschen");
        actions->setMenu(menu);
        connect(edit, &QAction::triggered, this, [this, context] { openContextDialog(context); });
        connect(remove, &QAction::triggered, this,
                [this, id = context.id] { apiClient.deleteContext(id); });
        top->addWidget(actions);
        cardLayout->addLayout(top);
        cardLayout->addStretch();

        auto *count = new QLabel(countText, card);
        count->setForegroundRole(QPalette::PlaceholderText);
        cardLayout->addWidget(count);

        grid->setItemWidget(item, card);
    }
    grid->doItemsLayout();
    grid->verticalScrollBar()->setValue(scrollPosition);
}

void ContextView::openContextDialog(std::optional<Context> context) {
    QDialog dialog(this);
    dialog.setWindowTitle(context ? "Kontext bearbeiten" : "Kontext erstellen");
    auto *form = new QFormLayout(&dialog);
    auto *title = new QLineEdit(context ? context->title : QString(), &dialog);
    title->setMaxLength(150);
    form->addRow("Titel", title);

    QByteArray icon = context ? context->icon : QByteArray();
    auto *iconRow = new QHBoxLayout;
    auto *selectIcon = new QPushButton(&dialog);
    auto *removeIcon = new QPushButton("Entfernen", &dialog);
    iconRow->addWidget(selectIcon, 1);
    iconRow->addWidget(removeIcon);
    form->addRow("Icon", iconRow);

    const auto updateIcon = [&] {
        selectIcon->setText(icon.isEmpty() ? "Icon auswählen" : "Icon ändern");
        selectIcon->setIcon(contextIcon(icon));
        removeIcon->setEnabled(!icon.isEmpty());
    };
    connect(selectIcon, &QPushButton::clicked, &dialog, [&] {
        const QString file = QFileDialog::getOpenFileName(
            &dialog, "Icon auswählen", {}, "PNG-Bilder (*.png)"
        );
        if (file.isEmpty()) {
            return;
        }
        const QPixmap selected(file, "PNG");
        if (!selected.isNull()) {
            icon.clear();
            QBuffer png(&icon);
            png.open(QIODevice::WriteOnly);
            selected.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(&png, "PNG");
            updateIcon();
        }
    });
    connect(removeIcon, &QPushButton::clicked, &dialog, [&] {
        icon.clear();
        updateIcon();
    });
    updateIcon();

    QColor color = context ? context->color : QColor();
    auto *colorRow = new QHBoxLayout;
    auto *selectColor = new QPushButton(&dialog);
    auto *removeColor = new QPushButton("Entfernen", &dialog);
    colorRow->addWidget(selectColor, 1);
    colorRow->addWidget(removeColor);
    form->addRow("Hintergrund", colorRow);

    const auto updateColor = [&] {
        selectColor->setText(color.isValid() ? color.name().toUpper() : "Farbe auswählen");
        QPixmap swatch;
        if (color.isValid()) {
            swatch = QPixmap(16, 16);
            swatch.fill(color);
        }
        selectColor->setIcon(QIcon(swatch));
        removeColor->setEnabled(color.isValid());
    };
    connect(selectColor, &QPushButton::clicked, &dialog, [&] {
        const QColor initial = color.isValid() ? color : dialog.palette().color(QPalette::Highlight);
        const QColor selected = QColorDialog::getColor(initial, &dialog, "Hintergrundfarbe");
        if (selected.isValid()) {
            color = selected;
            updateColor();
        }
    });
    connect(removeColor, &QPushButton::clicked, &dialog, [&] {
        color = QColor();
        updateColor();
    });
    updateColor();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText(context ? "Speichern" : "Erstellen");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    prepareFormDialog(dialog, *buttons);

    if (execDialog(dialog) != QDialog::Accepted || title->text().trimmed().isEmpty()) {
        return;
    }
    if (context) {
        apiClient.updateContext(context->id, title->text().trimmed(), icon, color);
    } else {
        apiClient.createContext(title->text().trimmed(), icon, color);
    }
}
