#include "SourceView.h"

#include "ContextMenuStyle.h"

#include "DialogUtils.h"
#include "../api/ApiClient.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr int ALL_TYPES = 0;
constexpr int domainRole = Qt::UserRole + 1;
constexpr int urlRole = Qt::UserRole + 2;
constexpr auto openIconPath = ":/assets/icons/open.png";

class SourceCardDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override {
        return {0, 58};
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        const QRect row = option.rect;
        const QRect action = openButtonRect(row);
        const int left = row.left() + 24;
        const int width = qMax(0, action.left() - left - 8);
        const bool hovered = option.state.testFlag(QStyle::State_MouseOver);
        const QColor foreground = option.palette.text().color();
        painter->save();
        painter->setClipRect(row);
        if (option.state.testFlag(QStyle::State_Selected)) {
            QColor selection = option.palette.highlight().color();
            selection.setAlpha(38);
            painter->fillRect(row, selection);
        }
        if (hovered) {
            painter->fillRect(row, QColor(255, 255, 255, 18));
        }

        auto font = option.font;
        font.setPixelSize(15);
        font.setWeight(QFont::DemiBold);
        painter->setFont(font);
        painter->setPen(foreground);
        constexpr int textFlags = Qt::AlignVCenter | Qt::TextSingleLine;
        painter->drawText(QRect(left, row.top() + 8, width, 22), textFlags,
                          QFontMetrics(font).elidedText(index.data().toString(), Qt::ElideRight, width));

        font.setPixelSize(13);
        font.setWeight(QFont::Normal);
        painter->setFont(font);
        QColor secondary = foreground;
        secondary.setAlpha(175);
        painter->setPen(secondary);
        painter->drawText(QRect(left, row.top() + 30, width, 18), textFlags,
                          QFontMetrics(font).elidedText(index.data(domainRole).toString(), Qt::ElideRight, width));

        painter->setOpacity(hovered ? 0.95 : 0.65);
        openIcon.paint(painter, action.adjusted(6, 6, -6, -6));
        painter->setOpacity(1.0);
        QColor separator = foreground;
        separator.setAlpha(34);
        painter->setPen(separator);
        painter->drawLine(row.bottomLeft(), row.bottomRight());
        painter->restore();
    }

    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override {
        const auto type = event->type();
        if (type != QEvent::MouseButtonPress && type != QEvent::MouseButtonRelease
            && type != QEvent::MouseButtonDblClick) {
            return QStyledItemDelegate::editorEvent(event, model, option, index);
        }
        const auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() != Qt::LeftButton) {
            return false;
        }

        const bool hit = openButtonRect(option.rect).contains(mouse->position().toPoint());
        if (type == QEvent::MouseButtonDblClick && !hit) {
            QDesktopServices::openUrl(QUrl(index.data(urlRole).toString()));
            return true;
        }
        if (type != QEvent::MouseButtonRelease) {
            pressedItem = hit ? index : QModelIndex{};
            return hit;
        }

        const bool pressedHere = pressedItem == index;
        pressedItem = QModelIndex{};
        if (pressedHere && hit) {
            QDesktopServices::openUrl(QUrl(index.data(urlRole).toString()));
        }
        return pressedHere || hit;
    }

private:
    static QRect openButtonRect(const QRect &row) {
        return {row.right() - 24 - 28 + 1, row.top() + 6, 28, 28};
    }

    QPersistentModelIndex pressedItem;
    const QIcon openIcon{openIconPath};
};
}

SourceView::SourceView(ApiClient &apiClient, QWidget *parent)
    : QWidget(parent), apiClient(apiClient) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *searchBar = new QWidget(this);
    searchBar->setObjectName("sourceSearchBar");
    searchBar->setStyleSheet(R"(
        QWidget#sourceSearchBar { border-bottom: 1px solid palette(midlight); }
        QLineEdit { background: transparent; border: 0; border-radius: 0; padding: 0; font-size: 14px; }
    )");
    auto *toolbar = new QHBoxLayout(searchBar);
    toolbar->setContentsMargins(24, 8, 24, 8);
    search = new QLineEdit(searchBar);
    search->setFrame(false);
    search->setMinimumHeight(42);
    search->setPlaceholderText("Quellen suchen");
    search->setAccessibleName("Quellen suchen");
    search->setClearButtonEnabled(true);
    typeFilter = new QComboBox(searchBar);
    toolbar->addWidget(search, 1);
    toolbar->addWidget(typeFilter);
    layout->addWidget(searchBar);

    list = new QListWidget(this);
    list->setAccessibleName("Quellen");
    list->setFrameShape(QFrame::NoFrame);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list->setContextMenuPolicy(Qt::CustomContextMenu);
    list->setMouseTracking(true);
    list->viewport()->setAttribute(Qt::WA_Hover);
    list->setItemDelegate(new SourceCardDelegate(list));
    list->setStyleSheet("QListWidget { background: transparent; border: 0; outline: 0; }");
    layout->addWidget(list);

    connect(search, &QLineEdit::textChanged, this, &SourceView::applyFilters);
    connect(typeFilter, &QComboBox::currentIndexChanged, this, &SourceView::applyFilters);
    connect(list, &QListWidget::customContextMenuRequested,
            this, &SourceView::showContextMenu);

    connect(&apiClient, &ApiClient::sourcesReceived, this,
            [this](const std::vector<Source> &received) {
                sources = received;
                rebuildList();
            });
    connect(&apiClient, &ApiClient::sourcesChanged, this, [this] {
        this->apiClient.getSources();
    });
    rebuildList();
    apiClient.getSources();
}

void SourceView::openCreateDialog() {
    const auto url = showTextInputDialog(this, "Quelle hinzufügen", "URL", "Hinzufügen");
    if (url) {
        apiClient.createSource(*url);
    }
}

void SourceView::rebuildList() {
    const QSignalBlocker blocker(typeFilter);
    const QVariant selectedType = typeFilter->currentData();
    typeFilter->clear();
    typeFilter->addItem("Alle Ressourcentypen", ALL_TYPES);

    QMap<QString, int> resourceTypes;
    list->clear();
    for (const Source &source : sources) {
        resourceTypes.insert(source.resourceType, source.resourceTypeId);
        auto *item = new QListWidgetItem(source.title, list);
        const QString domain = QUrl(source.url).host();
        item->setData(domainRole, domain);
        item->setData(urlRole, source.url);
        item->setData(Qt::AccessibleDescriptionRole, domain);
    }
    for (auto type = resourceTypes.cbegin(); type != resourceTypes.cend(); ++type) {
        typeFilter->addItem(type.key(), type.value());
    }
    typeFilter->setCurrentIndex(qMax(0, typeFilter->findData(selectedType)));
    applyFilters();
}

void SourceView::applyFilters() {
    const int typeId = typeFilter->currentData().toInt();
    const QString query = search->text().trimmed();
    for (int row = 0; row < list->count(); ++row) {
        auto *item = list->item(row);
        const bool matchesType = typeId == ALL_TYPES || sources[row].resourceTypeId == typeId;
        const bool matchesSearch = item->text().contains(query, Qt::CaseInsensitive)
            || item->data(domainRole).toString().contains(query, Qt::CaseInsensitive);
        item->setHidden(!matchesType || !matchesSearch);
    }
}

void SourceView::showContextMenu(const QPoint &position) {
    const QModelIndex index = list->indexAt(position);
    if (!index.isValid()) {
        return;
    }
    const Source source = sources[index.row()];
    QMenu menu(this);
    const auto *open = menu.addAction(QIcon(openIconPath), "Website öffnen");
    const auto *copy = menu.addAction(QIcon(":/assets/icons/copy.png"), "URL kopieren");
    const auto *edit = menu.addAction(QIcon(":/assets/icons/edit.png"), "Bearbeiten");
    menu.addSeparator();
    const auto *remove = menu.addAction(contextMenuDeleteIcon(), "Quelle löschen");
    const auto *selected = menu.exec(list->viewport()->mapToGlobal(position));
    if (selected == open) {
        QDesktopServices::openUrl(QUrl(source.url));
    } else if (selected == copy) {
        QApplication::clipboard()->setText(source.url);
    } else if (selected == edit) {
        if (const auto title = showTextInputDialog(
                this, "Quelle bearbeiten", "Titel", "Speichern", source.title)) {
            apiClient.updateSourceTitle(source.id, *title);
        }
    } else if (selected == remove && QMessageBox::question(
                   this, "Quelle löschen", "Diese Quelle löschen?",
                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        apiClient.deleteSource(source.id);
    }
}
