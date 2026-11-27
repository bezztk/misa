#include "ContextView.h"

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
#include <QToolButton>
#include <QVBoxLayout>

namespace {
constexpr QSize CARD_SIZE(268, 112);
constexpr int CARD_GAP = 12;
constexpr int CARD_MARGIN = 14;
constexpr int MENU_SIZE = 28;
constexpr int ICON_SIZE = 24;

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
    layout->setSpacing(16);

    auto *create = new QPushButton(QIcon(":/assets/icons/add.png"), "Kontext erstellen", this);
    layout->addWidget(create, 0, Qt::AlignLeft);

    grid->setAccessibleName("Kontexte");
    grid->setFrameShape(QFrame::NoFrame);
    grid->setViewMode(QListView::IconMode);
    grid->setFlow(QListView::LeftToRight);
    grid->setWrapping(true);
    grid->setResizeMode(QListView::Adjust);
    grid->setMovement(QListView::Static);
    grid->setGridSize(CARD_SIZE + QSize(CARD_GAP, CARD_GAP));
    grid->setUniformItemSizes(true);
    grid->setSelectionMode(QAbstractItemView::NoSelection);
    grid->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    grid->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    grid->setStyleSheet("QListWidget { background: transparent; }");
    layout->addWidget(grid);

    connect(create, &QPushButton::clicked, this, [this] { openContextDialog(std::nullopt); });
    connect(&apiClient, &ApiClient::contextsReceived, this, &ContextView::renderContexts);
    const auto refreshCounts = [this] { this->apiClient.getContexts(); };
    connect(&apiClient, &ApiClient::taskCreated, this, refreshCounts);
    connect(&apiClient, &ApiClient::taskUpdated, this, refreshCounts);
    connect(&apiClient, &ApiClient::taskDeleted, this, refreshCounts);
    apiClient.getContexts();
}

void ContextView::renderContexts(const std::vector<Context> &contexts) {
    const int scrollPosition = grid->verticalScrollBar()->value();
    grid->clear();
    for (const Context &context : contexts) {
        const QString countText = context.entityCount == 1
            ? "1 verknüpfter Eintrag"
            : QString("%1 verknüpfte Einträge").arg(context.entityCount);
        auto *item = new QListWidgetItem(grid);
        item->setSizeHint(CARD_SIZE);
        item->setData(Qt::AccessibleTextRole, context.title);
        item->setData(Qt::AccessibleDescriptionRole, countText);
        item->setToolTip(context.title);

        auto *card = new QFrame(grid->viewport());
        card->setObjectName("contextCard");
        card->setFixedSize(CARD_SIZE);
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
                font-size: 20px;
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
        const QIcon icon = contextIcon(context.icon);
        if (!icon.isNull()) {
            auto *iconLabel = new QLabel(card);
            iconLabel->setPixmap(icon.pixmap(QSize(ICON_SIZE, ICON_SIZE), card->devicePixelRatioF()));
            top->addWidget(iconLabel);
        }
        top->addStretch(1);
        auto *actions = new QToolButton(card);
        actions->setText("⋯");
        actions->setAccessibleName("Aktionen für " + context.title);
        actions->setToolTip("Kontextaktionen");
        actions->setFixedSize(MENU_SIZE, MENU_SIZE);
        actions->setPopupMode(QToolButton::InstantPopup);
        auto *menu = new QMenu(actions);
        auto *edit = menu->addAction(QIcon(":/assets/icons/edit.png"), "Bearbeiten");
        auto *remove = menu->addAction(QIcon(":/assets/icons/delete.png"), "Löschen");
        actions->setMenu(menu);
        connect(edit, &QAction::triggered, this, [this, context] { openContextDialog(context); });
        connect(remove, &QAction::triggered, this,
                [this, id = context.id] { apiClient.deleteContext(id); });
        top->addWidget(actions);
        cardLayout->addLayout(top);

        auto *title = new QLabel(card);
        title->setTextFormat(Qt::PlainText);
        auto titleFont = title->font();
        titleFont.setPixelSize(22);
        titleFont.setWeight(QFont::DemiBold);
        title->setFont(titleFont);
        title->setText(QFontMetrics(titleFont).elidedText(
            context.title, Qt::ElideRight, CARD_SIZE.width() - 2 * CARD_MARGIN
        ));
        cardLayout->addWidget(title);

        auto *count = new QLabel(countText, card);
        count->setForegroundRole(QPalette::PlaceholderText);
        cardLayout->addWidget(count);
        cardLayout->addStretch();

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
    form->addRow(buttons);
    prepareDialog(dialog);

    if (execDialog(dialog) != QDialog::Accepted || title->text().trimmed().isEmpty()) {
        return;
    }
    if (context) {
        apiClient.updateContext(context->id, title->text().trimmed(), icon, color);
    } else {
        apiClient.createContext(title->text().trimmed(), icon, color);
    }
}
