#include "RssView.h"

#include "../api/ApiClient.h"
#include "DialogUtils.h"
#include "NavigationListDelegate.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTextDocumentFragment>
#include <QTextLayout>
#include <QToolButton>
#include <QUrl>

#include <algorithm>
#include <functional>

namespace {

constexpr int allFeedsId = 0;
constexpr int feedIdRole = Qt::UserRole;
enum ArticleRole { EntryIdRole = Qt::UserRole, GroupRole, SourceRole, TimeRole, UrlRole, ReadRole, SummaryRole };

constexpr int articleInset = 22;
constexpr int articlePadding = 10;
constexpr int actionSize = 24;

QString displayedTitle(const QString &title) {
    const QUrl url(title);
    return (url.scheme() == "https" || url.scheme() == "http") && !url.host().isEmpty()
        ? url.host() : title;
}

QString articleSummary(const QString &html) {
    static const QRegularExpression url(R"(\bhttps?://[^\s<>]+)", QRegularExpression::CaseInsensitiveOption);
    return QTextDocumentFragment::fromHtml(html).toPlainText().remove(url).simplified();
}

QStringList wrappedLines(const QString &text, const QFont &font, const int width, const int maxLines) {
    QStringList lines;
    if (text.isEmpty()) {
        return lines;
    }

    const int lineWidth = std::max(1, width);
    QTextLayout layout(text, font);
    QTextOption options;
    options.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    layout.setTextOption(options);
    layout.beginLayout();
    for (int i = 0; i < maxLines; ++i) {
        QTextLine line = layout.createLine();
        if (!line.isValid()) {
            break;
        }
        line.setLineWidth(lineWidth);
        const int start = line.textStart();
        if (i + 1 == maxLines && start + line.textLength() < text.size()) {
            lines.append(QFontMetrics(font).elidedText(text.mid(start), Qt::ElideRight, lineWidth));
        } else {
            lines.append(text.mid(start, line.textLength()).trimmed());
        }
    }
    layout.endLayout();
    return lines;
}

struct ArticleText {
    ArticleText(const QFont &font, const QModelIndex &index, const int width)
        : titleFont(font), metadataFont(font), summaryFont(font) {
        titleFont.setPixelSize(15);
        titleFont.setWeight(index.data(ReadRole).toBool() ? QFont::Normal : QFont::DemiBold);
        metadataFont.setPixelSize(12);
        summaryFont.setPixelSize(14);

        title = wrappedLines(index.data().toString(), titleFont, width - 2 * articleInset - 2 * actionSize - 8, 2);
        summary = wrappedLines(index.data(SummaryRole).toString(), summaryFont, width - 2 * articleInset, 4);
        titleHeight = QFontMetrics(titleFont).lineSpacing() * std::max(1, static_cast<int>(title.size()));
        metadataHeight = QFontMetrics(metadataFont).height();
        summaryHeight = QFontMetrics(summaryFont).lineSpacing() * static_cast<int>(summary.size());
    }

    int height() const {
        return 2 * articlePadding + titleHeight + 4 + metadataHeight + (summaryHeight ? 8 + summaryHeight : 0);
    }

    QFont titleFont;
    QFont metadataFont;
    QFont summaryFont;
    QStringList title;
    QStringList summary;
    int titleHeight;
    int metadataHeight;
    int summaryHeight;
};

class ArticleDelegate final : public QStyledItemDelegate {
public:
    ArticleDelegate(QListWidget &list, ApiClient &apiClient)
        : QStyledItemDelegate(&list), list(list), apiClient(apiClient) {}

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        if (!index.data(EntryIdRole).isValid()) {
            return QStyledItemDelegate::sizeHint(option, index);
        }
        return {0, ArticleText(option.font, index, list.viewport()->width()).height()};
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        if (!index.data(EntryIdRole).isValid()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        const QRect bounds = option.rect;
        const ArticleText text(option.font, index, bounds.width());
        const bool read = index.data(ReadRole).toBool();
        const bool hovered = option.state.testFlag(QStyle::State_MouseOver);
        const QColor foreground = option.palette.text().color();

        painter->save();
        painter->setClipRect(bounds);
        if (!read) {
            painter->fillRect(bounds, QColor(127, 167, 226, 16));
            painter->fillRect(bounds.left(), bounds.top(), 2, bounds.height(), QColor("#7FA7E2"));
        }
        if (hovered || text.summaryHeight) {
            painter->fillRect(bounds, QColor(255, 255, 255, hovered ? 14 : 8));
        }
        if (option.state.testFlag(QStyle::State_HasFocus)) {
            QColor focus = foreground;
            focus.setAlpha(66);
            painter->setPen(focus);
            painter->drawRect(bounds.adjusted(0, 0, -1, -1));
        }

        const int left = bounds.left() + articleInset;
        const int width = bounds.width() - 2 * articleInset;
        const int top = bounds.top() + articlePadding;
        painter->setPen(foreground);
        if (width > 0) {
            drawLines(*painter, text.titleFont, text.title, left, top, width);

            const int metadataTop = top + text.titleHeight + 4;
            const QString time = index.data(TimeRole).toString();
            const QFontMetrics metadataMetrics(text.metadataFont);
            const int timeWidth = metadataMetrics.horizontalAdvance(time);
            const int sourceWidth = std::max(0, width - timeWidth - 12);
            QColor secondary = foreground;
            secondary.setAlpha(170);
            painter->setFont(text.metadataFont);
            painter->setPen(secondary);
            painter->drawText(QRect(left, metadataTop, sourceWidth, text.metadataHeight),
                              Qt::AlignVCenter | Qt::TextSingleLine,
                              metadataMetrics.elidedText(index.data(SourceRole).toString(), Qt::ElideRight, sourceWidth));
            painter->drawText(QRect(left, metadataTop, width, text.metadataHeight),
                              Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine, time);

            if (text.summaryHeight) {
                secondary.setAlpha(210);
                painter->setPen(secondary);
                drawLines(*painter, text.summaryFont, text.summary,
                          left, metadataTop + text.metadataHeight + 8, width);
            }
        }

        if (hovered) {
            drawEye(*painter, actionRect(bounds, Action::Read), read);
            openIcon.paint(painter, actionRect(bounds, Action::Open).adjusted(5, 5, -5, -5),
                           Qt::AlignCenter, index.data(UrlRole).toString().isEmpty() ? QIcon::Disabled : QIcon::Normal);
        }

        QColor separator = foreground;
        separator.setAlpha(34);
        painter->setPen(separator);
        painter->drawLine(bounds.bottomLeft(), bounds.bottomRight());
        painter->restore();
    }

    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override {
        const auto type = event->type();
        if (!index.data(EntryIdRole).isValid()
            || (type != QEvent::MouseButtonPress && type != QEvent::MouseButtonRelease
                && type != QEvent::MouseButtonDblClick)) {
            return QStyledItemDelegate::editorEvent(event, model, option, index);
        }
        const auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() != Qt::LeftButton) {
            return false;
        }

        const QPoint point = mouse->position().toPoint();
        const bool hitsRead = actionRect(option.rect, Action::Read).contains(point);
        const bool hitsOpen = actionRect(option.rect, Action::Open).contains(point);
        const bool hit = hitsRead || hitsOpen;
        const Action action = hitsRead ? Action::Read : Action::Open;
        if (type == QEvent::MouseButtonDblClick && !hit) {
            openArticle(index);
            return true;
        }
        if (type != QEvent::MouseButtonRelease) {
            pressedItem = hit ? index : QModelIndex{};
            pressedAction = action;
            return hit;
        }

        const bool pressedHere = pressedItem == index;
        pressedItem = QModelIndex{};
        if (pressedHere && hit && pressedAction == action) {
            if (action == Action::Read) {
                apiClient.updateRssEntryRead(index.data(EntryIdRole).toInt(), !index.data(ReadRole).toBool());
            } else {
                openArticle(index);
            }
        }
        return pressedHere || hit;
    }

private:
    enum class Action { Read, Open };

    static void openArticle(const QModelIndex &index) {
        const QString url = index.data(UrlRole).toString();
        if (!url.isEmpty()) {
            QDesktopServices::openUrl(QUrl(url));
        }
    }

    static QRect actionRect(const QRect &bounds, const Action action) {
        const int offset = action == Action::Open ? actionSize : 2 * actionSize;
        return {bounds.right() + 1 - articleInset - offset, bounds.top() + 6, actionSize, actionSize};
    }

    static void drawLines(QPainter &painter, const QFont &font, const QStringList &lines,
                          const int x, int y, const int width) {
        painter.setFont(font);
        const int lineHeight = QFontMetrics(font).lineSpacing();
        for (const QString &line : lines) {
            painter.drawText(QRect(x, y, width, lineHeight), Qt::AlignVCenter | Qt::TextSingleLine, line);
            y += lineHeight;
        }
    }

    static void drawEye(QPainter &painter, const QRect &bounds, const bool crossedOut) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(bounds.center());
        painter.setPen(QPen(QColor("#B8B8B8"), 1.25, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);

        QPainterPath eye;
        eye.moveTo(-7, 0);
        eye.cubicTo(-3, -6, 3, -6, 7, 0);
        eye.cubicTo(3, 6, -3, 6, -7, 0);
        painter.drawPath(eye);
        painter.drawEllipse(QPointF(0, 0), 2, 2);
        if (crossedOut) {
            painter.drawLine(QPointF(-6, -5), QPointF(6, 5));
        }
        painter.restore();
    }

    QListWidget &list;
    ApiClient &apiClient;
    QPersistentModelIndex pressedItem;
    Action pressedAction = Action::Read;
    const QIcon openIcon{":/assets/icons/open.png"};
};

class DateHeader final : public QWidget {
public:
    DateHeader(
        const QString &text,
        const bool collapsed,
        std::function<void(bool)> toggled,
        QWidget *parent
    ) : QWidget(parent), collapsed(collapsed), toggled(std::move(toggled)) {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(15, 8, articleInset, 4);
        layout->setSpacing(6);
        button = new QToolButton(this);
        button->setArrowType(collapsed ? Qt::RightArrow : Qt::DownArrow);
        button->setAutoRaise(true);
        button->setAccessibleName(text);
        button->setCursor(Qt::PointingHandCursor);
        button->setFixedSize(16, 16);
        button->setStyleSheet("QToolButton { border: none; padding: 0; margin: 0; }");
        layout->addWidget(button);
        auto *label = new QLabel(text, this);
        QFont font = label->font();
        font.setBold(true);
        label->setFont(font);
        layout->addWidget(label);
        auto *separator = new QFrame(this);
        separator->setFixedHeight(1);
        separator->setStyleSheet("background: #505050; border: none;");
        layout->addWidget(separator, 1);
        connect(button, &QToolButton::clicked, this, [this] {
            this->collapsed = !this->collapsed;
            button->setArrowType(
                this->collapsed ? Qt::RightArrow : Qt::DownArrow
            );
            this->toggled(this->collapsed);
        });
    }

private:
    bool collapsed;
    std::function<void(bool)> toggled;
    QToolButton *button;
};

} // namespace

RssView::RssView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client), navigation(new QListWidget(this)), articleList(new QListWidget(this)) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QFont navigationFont(QStringLiteral("Segoe UI"));
    navigationFont.setPixelSize(15);
    navigationFont.setWeight(QFont::Medium);
    navigation->setFont(navigationFont);
    navigation->setFixedWidth(300);
    navigation->setAccessibleName("Feeds");
    navigation->setMouseTracking(true);
    navigation->viewport()->setAttribute(Qt::WA_Hover);
    navigation->setItemDelegate(new NavigationListDelegate(navigation));
    navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigation->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    navigation->setContextMenuPolicy(Qt::CustomContextMenu);
    navigation->setStyleSheet(R"(
        QListWidget {
            background: transparent;
            border: 0;
            border-right: 1px solid palette(midlight);
            outline: 0;
            padding: 8px 0;
        }
    )");
    layout->addWidget(navigation);

    articleList->setAccessibleName("Beiträge");
    articleList->setFrameShape(QFrame::NoFrame);
    articleList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    articleList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    articleList->setResizeMode(QListView::Adjust);
    articleList->setSpacing(0);
    articleList->setMouseTracking(true);
    articleList->viewport()->setAttribute(Qt::WA_Hover);
    articleList->setItemDelegate(new ArticleDelegate(*articleList, apiClient));
    articleList->setStyleSheet("QListWidget { background: transparent; border: 0; outline: 0; }");
    layout->addWidget(articleList, 1);

    connect(navigation, &QListWidget::currentItemChanged, this, [this] {
        renderArticles();
        articleList->scrollToTop();
    });
    connect(navigation, &QListWidget::customContextMenuRequested, this, &RssView::showNavigationContextMenu);
    connect(articleList, &QListWidget::itemClicked, this, &RssView::toggleArticle);
    connect(&apiClient, &ApiClient::rssFeedsReceived, this, [this](const std::vector<RssFeed> &received) {
        feeds = received;
        renderNavigation();
        renderArticles();
    });
    connect(&apiClient, &ApiClient::rssEntriesReceived, this, [this](const std::vector<RssEntry> &received) {
        entries = received;
        updateUnreadCounts();
        renderArticles();
    });

    renderNavigation();
}

void RssView::refresh() {
    apiClient.getRssFeeds();
    apiClient.getRssEntries();
}

void RssView::renderNavigation() {
    const int selectedId = navigation->currentItem() ? navigation->currentItem()->data(feedIdRole).toInt() : allFeedsId;
    const int scrollPosition = navigation->verticalScrollBar()->value();
    const QSignalBlocker blocker(navigation);
    navigation->clear();

    const auto addFeed = [this](const QString &title, const int id) {
        auto *item = new QListWidgetItem(title, navigation);
        item->setData(feedIdRole, id);
        item->setSizeHint(QSize(0, 44));
        return item;
    };
    QListWidgetItem *selected = addFeed("Alle Feeds", allFeedsId);
    for (const RssFeed &feed : feeds) {
        const QString title = displayedTitle(feed.title);
        auto *item = addFeed(feed.enabled ? title : title + " – deaktiviert", feed.id);
        if (feed.id == selectedId) {
            selected = item;
        }
    }
    updateUnreadCounts();
    navigation->setCurrentItem(selected);
    navigation->doItemsLayout();
    navigation->verticalScrollBar()->setValue(scrollPosition);
}

void RssView::updateUnreadCounts() {
    QHash<int, int> counts;
    int total = 0;
    for (const RssEntry &entry : entries) {
        if (!entry.isRead) {
            ++counts[entry.feedId];
            ++total;
        }
    }
    for (int row = 0; row < navigation->count(); ++row) {
        auto *item = navigation->item(row);
        const int id = item->data(feedIdRole).toInt();
        const int count = id == allFeedsId ? total : counts.value(id);
        item->setData(NavigationListDelegate::CountRole, count);
        item->setData(Qt::AccessibleDescriptionRole,
                      count == 1 ? "1 ungelesener Beitrag" : QString("%1 ungelesene Beiträge").arg(count));
    }
}

void RssView::renderArticles() {
    const int feedId = navigation->currentItem()->data(feedIdRole).toInt();
    const int scrollPosition = articleList->verticalScrollBar()->value();
    const QSignalBlocker blocker(articleList);
    articleList->clear();

    QString currentGroup;
    const QLocale german(QLocale::German, QLocale::Germany);
    for (const RssEntry &entry : entries) {
        if ((feedId != allFeedsId && entry.feedId != feedId)
            || std::ranges::find(feeds, entry.feedId, &RssFeed::id) == feeds.end()) {
            continue;
        }

        const QDateTime published = entry.publishedAt ? entry.publishedAt->toLocalTime() : QDateTime();
        const QDate date = published.date();
        const QString group = date.isValid() ? date.toString(Qt::ISODate) : QString("without-date");
        if (group != currentGroup) {
            currentGroup = group;
            const QString groupTitle = date.isValid() ? german.toString(date, "dddd, d. MMMM") : QString("Ohne Datum");
            auto *header = new QListWidgetItem(articleList);
            header->setFlags(header->flags() & ~Qt::ItemIsSelectable);
            header->setSizeHint(QSize(0, 36));
            articleList->setItemWidget(header, new DateHeader(
                groupTitle, collapsedArticleGroups.contains(group),
                [this, group](const bool collapsed) {
                    if (collapsed) {
                        collapsedArticleGroups.insert(group);
                    } else {
                        collapsedArticleGroups.remove(group);
                    }
                    for (int row = 0; row < articleList->count(); ++row) {
                        auto *item = articleList->item(row);
                        if (item->data(GroupRole).toString() == group) {
                            item->setHidden(collapsed);
                        }
                    }
                },
                articleList
            ));
        }

        auto *item = new QListWidgetItem(displayedTitle(entry.title).simplified(), articleList);
        item->setData(EntryIdRole, entry.id);
        item->setData(GroupRole, group);
        item->setData(SourceRole, displayedTitle(entry.feedTitle));
        item->setData(TimeRole, published.toString("HH:mm"));
        item->setData(UrlRole, entry.url);
        item->setData(ReadRole, entry.isRead);
        item->setData(Qt::AccessibleDescriptionRole, entry.isRead ? "Gelesen" : "Ungelesen");
        if (expandedArticles.contains(entry.id)) {
            item->setData(SummaryRole, articleSummary(entry.summary));
        }
        item->setHidden(collapsedArticleGroups.contains(group));
    }

    articleList->doItemsLayout();
    articleList->verticalScrollBar()->setValue(scrollPosition);
}

void RssView::toggleArticle(QListWidgetItem *item) {
    if (!item->data(EntryIdRole).isValid()) {
        return;
    }
    const int id = item->data(EntryIdRole).toInt();
    if (expandedArticles.remove(id)) {
        item->setData(SummaryRole, QString());
    } else {
        const auto entry = std::ranges::find(entries, id, &RssEntry::id);
        const QString summary = articleSummary(entry->summary);
        if (summary.isEmpty()) {
            return;
        }
        expandedArticles.insert(id);
        item->setData(SummaryRole, summary);
    }
    articleList->doItemsLayout();
}

void RssView::openCreateFeedDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Feed abonnieren");
    auto *form = new QFormLayout(&dialog);
    auto *urlEdit = new QLineEdit(&dialog);
    urlEdit->setPlaceholderText("https://example.com/feed.xml");
    form->addRow("URL", urlEdit);
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog
    );
    buttons->button(QDialogButtonBox::Save)->setText("Abonnieren");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    prepareFormDialog(dialog, *buttons);
    if (execDialog(dialog) != QDialog::Accepted || urlEdit->text().trimmed().isEmpty()) {
        return;
    }
    apiClient.createRssFeed(urlEdit->text().trimmed());
}

void RssView::openFeedDialog(const RssFeed &feed) {
    QDialog dialog(this);
    dialog.setWindowTitle("Feed bearbeiten");
    auto *form = new QFormLayout(&dialog);
    auto *urlEdit = new QLineEdit(feed.url, &dialog);
    form->addRow("URL", urlEdit);
    auto *enabledEdit = new QCheckBox("Aktiv", &dialog);
    enabledEdit->setChecked(feed.enabled);
    form->addRow("Status", enabledEdit);
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog
    );
    buttons->button(QDialogButtonBox::Save)->setText("Speichern");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    prepareFormDialog(dialog, *buttons);
    if (execDialog(dialog) != QDialog::Accepted || urlEdit->text().trimmed().isEmpty()) {
        return;
    }
    apiClient.updateRssFeed(feed.id, urlEdit->text().trimmed(), enabledEdit->isChecked());
}

void RssView::showNavigationContextMenu(const QPoint &position) {
    const auto *item = navigation->itemAt(position);
    if (!item) {
        return;
    }
    const auto feed = std::ranges::find(feeds, item->data(feedIdRole).toInt(), &RssFeed::id);
    if (feed == feeds.end()) {
        return;
    }
    const RssFeed selectedFeed = *feed;

    QMenu menu(this);
    QAction *refreshAction = menu.addAction("Aktualisieren");
    refreshAction->setEnabled(selectedFeed.enabled);
    QAction *readAction = menu.addAction("Als gelesen markieren");
    QAction *editAction = menu.addAction("Bearbeiten");
    menu.addSeparator();
    QAction *hideAction = menu.addAction("Ausblenden");
    QAction *selected = menu.exec(navigation->viewport()->mapToGlobal(position));
    if (selected == refreshAction) {
        apiClient.refreshRssFeed(selectedFeed.id);
    } else if (selected == readAction) {
        apiClient.markRssFeedRead(selectedFeed.id);
    } else if (selected == editAction) {
        openFeedDialog(selectedFeed);
    } else if (selected == hideAction) {
        apiClient.deleteRssFeed(selectedFeed.id);
    }
}
