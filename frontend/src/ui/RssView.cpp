#include "RssView.h"

#include <QAbstractItemView>
#include <QDesktopServices>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTextBrowser>
#include <QTextDocumentFragment>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

RssView::RssView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    auto *subscribeRow = new QHBoxLayout();
    urlEdit = new QLineEdit(this);
    urlEdit->setPlaceholderText("Feed-URL");
    auto *subscribeButton = new QPushButton("Abonnieren", this);
    subscribeRow->addWidget(urlEdit, 1);
    subscribeRow->addWidget(subscribeButton);
    layout->addLayout(subscribeRow);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    auto *feedsPane = new QWidget(splitter);
    auto *feedsLayout = new QVBoxLayout(feedsPane);
    feedsLayout->setContentsMargins(0, 0, 0, 0);
    feedsLayout->addWidget(new QLabel("Abonnements", feedsPane));
    feedList = new QListWidget(feedsPane);
    feedList->setContextMenuPolicy(Qt::CustomContextMenu);
    feedsLayout->addWidget(feedList, 1);

    auto *entriesPane = new QWidget(splitter);
    auto *entriesLayout = new QVBoxLayout(entriesPane);
    entriesLayout->setContentsMargins(0, 0, 0, 0);
    entryList = new QTreeWidget(entriesPane);
    entryList->setHeaderLabels({"Titel", "Feed", "Veröffentlicht"});
    entryList->setRootIsDecorated(false);
    entryList->setSelectionMode(QAbstractItemView::SingleSelection);
    entryList->setContextMenuPolicy(Qt::CustomContextMenu);
    entryList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    entryList->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    entryList->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    entriesLayout->addWidget(entryList, 3);

    auto *detail = new QWidget(entriesPane);
    auto *detailLayout = new QVBoxLayout(detail);
    detailLayout->setContentsMargins(0, 8, 0, 0);
    detailTitle = new QLabel(detail);
    QFont titleFont = detailTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    detailTitle->setFont(titleFont);
    detailTitle->setWordWrap(true);
    detailLayout->addWidget(detailTitle);
    detailDate = new QLabel(detail);
    detailDate->setStyleSheet("color: #999999;");
    detailLayout->addWidget(detailDate);
    detailUrl = new QLabel(detail);
    detailUrl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailUrl->setStyleSheet("color: #8AB4F8;");
    detailUrl->setWordWrap(true);
    detailLayout->addWidget(detailUrl);
    detailSummary = new QTextBrowser(detail);
    detailSummary->setOpenExternalLinks(false);
    detailLayout->addWidget(detailSummary, 1);
    auto *detailButtons = new QHBoxLayout();
    openButton = new QPushButton("Im Browser öffnen", detail);
    readButton = new QPushButton(detail);
    detailButtons->addWidget(openButton);
    detailButtons->addWidget(readButton);
    detailButtons->addStretch();
    detailLayout->addLayout(detailButtons);
    entriesLayout->addWidget(detail, 2);

    splitter->addWidget(feedsPane);
    splitter->addWidget(entriesPane);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({260, 900});
    layout->addWidget(splitter, 1);

    connect(subscribeButton, &QPushButton::clicked, this, &RssView::subscribe);
    connect(urlEdit, &QLineEdit::returnPressed, this, &RssView::subscribe);
    connect(feedList, &QListWidget::currentItemChanged, this,
            [this] { renderEntries(); });
    connect(feedList, &QListWidget::customContextMenuRequested,
            this, &RssView::showFeedContextMenu);
    connect(entryList, &QTreeWidget::currentItemChanged, this,
            [this] { renderDetail(); });
    connect(entryList, &QTreeWidget::itemDoubleClicked, this,
            [this] { openSelectedEntry(); });
    connect(entryList, &QTreeWidget::customContextMenuRequested,
            this, &RssView::showEntryContextMenu);
    connect(openButton, &QPushButton::clicked, this, &RssView::openSelectedEntry);
    connect(readButton, &QPushButton::clicked, this,
            &RssView::toggleSelectedEntryRead);

    connect(&apiClient, &ApiClient::rssFeedsReceived, this,
            [this](const std::vector<RssFeed> &received) {
                feeds = received;
                renderFeeds();
            });
    connect(&apiClient, &ApiClient::rssEntriesReceived, this,
            [this](const std::vector<RssEntry> &received) {
                entries = received;
                renderEntries();
            });
    connect(&apiClient, &ApiClient::rssFeedCreated, this, [this] {
        urlEdit->clear();
        refresh();
    });
    connect(&apiClient, &ApiClient::rssFeedUpdated, this,
            [this] { apiClient.getRssFeeds(); });
    connect(&apiClient, &ApiClient::rssFeedDeleted, this, &RssView::refresh);
    connect(&apiClient, &ApiClient::rssEntryUpdated, this,
            [this] { apiClient.getRssEntries(); });

    renderDetail();
}

void RssView::refresh() {
    apiClient.getRssFeeds();
    apiClient.getRssEntries();
}

void RssView::subscribe() {
    const QString url = urlEdit->text().trimmed();
    if (!url.isEmpty()) {
        apiClient.createRssFeed(url);
    }
}

void RssView::renderFeeds() {
    const int selectedFeedId = feedList->currentItem()
        ? feedList->currentItem()->data(Qt::UserRole).toInt()
        : 0;
    const QSignalBlocker blocker(feedList);
    feedList->clear();
    auto *allItem = new QListWidgetItem("Alle Feeds", feedList);
    allItem->setData(Qt::UserRole, 0);
    QListWidgetItem *selection = selectedFeedId == 0 ? allItem : nullptr;
    for (const RssFeed &feed : feeds) {
        QString title = feed.title;
        if (!feed.enabled) {
            title += " (deaktiviert)";
        }
        auto *item = new QListWidgetItem(title, feedList);
        item->setData(Qt::UserRole, feed.id);
        if (feed.id == selectedFeedId) {
            selection = item;
        }
    }
    feedList->setCurrentItem(selection ? selection : allItem);
    renderEntries();
}

void RssView::renderEntries() {
    const int selectedFeedId = feedList->currentItem()
        ? feedList->currentItem()->data(Qt::UserRole).toInt()
        : 0;
    const int selectedEntryId = selectedEntry() ? selectedEntry()->id : 0;
    const QSignalBlocker blocker(entryList);
    entryList->clear();
    QTreeWidgetItem *selection = nullptr;
    for (const RssEntry &entry : entries) {
        if (selectedFeedId != 0 && entry.feedId != selectedFeedId) {
            continue;
        }
        const QString published = entry.publishedAt.has_value()
            ? entry.publishedAt->toLocalTime().toString("dd.MM.yyyy HH:mm")
            : QString();
        auto *item = new QTreeWidgetItem(entryList, {
            entry.title, entry.feedTitle, published
        });
        item->setData(0, Qt::UserRole, entry.id);
        if (!entry.isRead) {
            for (int column = 0; column < 3; ++column) {
                QFont font = item->font(column);
                font.setBold(true);
                item->setFont(column, font);
            }
        }
        if (entry.id == selectedEntryId) {
            selection = item;
        }
    }
    if (!selection && entryList->topLevelItemCount() > 0) {
        selection = entryList->topLevelItem(0);
    }
    entryList->setCurrentItem(selection);
    renderDetail();
}

void RssView::renderDetail() {
    RssEntry *entry = selectedEntry();
    const bool hasEntry = entry != nullptr;
    detailTitle->setText(hasEntry ? entry->title : "Kein Eintrag ausgewählt");
    detailDate->setText(
        hasEntry && entry->publishedAt.has_value()
            ? entry->publishedAt->toLocalTime().toString("dd.MM.yyyy HH:mm")
            : QString()
    );
    detailUrl->setText(hasEntry ? entry->url : QString());
    detailSummary->setPlainText(
        hasEntry
            ? QTextDocumentFragment::fromHtml(entry->summary).toPlainText()
            : QString()
    );
    openButton->setEnabled(hasEntry && !entry->url.isEmpty());
    readButton->setEnabled(hasEntry);
    readButton->setText(
        hasEntry && entry->isRead ? "Als ungelesen markieren" : "Als gelesen markieren"
    );
}

RssEntry *RssView::selectedEntry() {
    if (!entryList->currentItem()) {
        return nullptr;
    }
    const int id = entryList->currentItem()->data(0, Qt::UserRole).toInt();
    const auto found = std::find_if(entries.begin(), entries.end(),
                                    [id](const RssEntry &entry) {
                                        return entry.id == id;
                                    });
    return found == entries.end() ? nullptr : &*found;
}

RssFeed *RssView::feedById(const int id) {
    const auto found = std::find_if(feeds.begin(), feeds.end(),
                                    [id](const RssFeed &feed) {
                                        return feed.id == id;
                                    });
    return found == feeds.end() ? nullptr : &*found;
}

void RssView::openSelectedEntry() {
    RssEntry *entry = selectedEntry();
    if (entry && !entry->url.isEmpty()) {
        QDesktopServices::openUrl(QUrl(entry->url));
    }
}

void RssView::toggleSelectedEntryRead() {
    RssEntry *entry = selectedEntry();
    if (entry) {
        apiClient.updateRssEntryRead(entry->id, !entry->isRead);
    }
}

void RssView::showFeedContextMenu(const QPoint &position) {
    QListWidgetItem *item = feedList->itemAt(position);
    if (!item) {
        return;
    }
    RssFeed *feed = feedById(item->data(Qt::UserRole).toInt());
    if (!feed) {
        return;
    }

    QMenu menu(this);
    QAction *enabledAction = menu.addAction(
        feed->enabled ? "Deaktivieren" : "Aktivieren"
    );
    QAction *hideAction = menu.addAction("Abonnement ausblenden");
    QAction *selected = menu.exec(feedList->mapToGlobal(position));
    if (selected == enabledAction) {
        apiClient.updateRssFeedEnabled(feed->id, !feed->enabled);
    } else if (selected == hideAction) {
        apiClient.deleteRssFeed(feed->id);
    }
}

void RssView::showEntryContextMenu(const QPoint &position) {
    QTreeWidgetItem *item = entryList->itemAt(position);
    if (!item) {
        return;
    }
    entryList->setCurrentItem(item);
    RssEntry *entry = selectedEntry();
    if (!entry) {
        return;
    }

    QMenu menu(this);
    QAction *openAction = menu.addAction("Im Browser öffnen");
    openAction->setEnabled(!entry->url.isEmpty());
    QAction *readAction = menu.addAction(
        entry->isRead ? "Als ungelesen markieren" : "Als gelesen markieren"
    );
    QAction *selected = menu.exec(entryList->viewport()->mapToGlobal(position));
    if (selected == openAction) {
        openSelectedEntry();
    } else if (selected == readAction) {
        toggleSelectedEntryRead();
    }
}
