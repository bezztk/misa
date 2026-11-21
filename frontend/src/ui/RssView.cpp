#include "RssView.h"

#include "DialogUtils.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTextBrowser>
#include <QTextDocumentFragment>
#include <QTextLayout>
#include <QToolButton>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

namespace {
constexpr int NAV_KIND_ROLE = Qt::UserRole;
constexpr int NAV_ID_ROLE = Qt::UserRole + 1;
constexpr int ENTRY_ID_ROLE = Qt::UserRole;
constexpr int GROUP_KEY_ROLE = Qt::UserRole + 1;

enum NavigationKind {
    AllFeeds,
    Category,
    Feed,
    Unassigned
};

class ArticleCard final : public QFrame {
public:
    ArticleCard(
        const RssEntry &entry,
        std::function<void(int)> heightChanged,
        QWidget *parent
    ) : QFrame(parent),
        fullTitle(entry.title),
        unread(!entry.isRead),
        heightChanged(std::move(heightChanged)) {
        setObjectName("rssArticleCard");
        setFixedHeight(76);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_TransparentForMouseEvents);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(10, 7, 10, 7);
        layout->setSpacing(3);
        title = new QLabel(this);
        title->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        layout->addWidget(title);

        auto *metaRow = new QWidget(this);
        auto *metaLayout = new QHBoxLayout(metaRow);
        metaLayout->setContentsMargins(0, 0, 0, 0);
        metaLayout->setSpacing(8);
        auto *source = new QLabel(entry.feedTitle, metaRow);
        source->setStyleSheet("color: #AAAAAA; background: transparent;");
        source->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        source->setTextInteractionFlags(Qt::NoTextInteraction);
        source->setToolTip(entry.feedTitle);
        metaLayout->addWidget(source, 1);
        auto *time = new QLabel(
            entry.publishedAt.has_value()
                ? entry.publishedAt->toLocalTime().toString("HH:mm")
                : QString(),
            metaRow
        );
        time->setStyleSheet("color: #888888; background: transparent;");
        metaLayout->addWidget(time);
        layout->addWidget(metaRow);
        applyStyle();
    }

    void setSelected(const bool value) {
        if (selected == value) {
            return;
        }
        selected = value;
        applyStyle();
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QFrame::resizeEvent(event);
        updateTitle();
    }

private:
    void updateTitle() {
        const int width = qMax(1, title->width());
        QTextLayout textLayout(fullTitle, title->font());
        textLayout.beginLayout();
        QTextLine firstLine = textLayout.createLine();
        if (!firstLine.isValid()) {
            textLayout.endLayout();
            title->clear();
            return;
        }
        firstLine.setLineWidth(width);
        QTextLine secondLine = textLayout.createLine();
        if (secondLine.isValid()) {
            secondLine.setLineWidth(width);
        }
        textLayout.endLayout();

        const QString first = fullTitle.mid(
            firstLine.textStart(), firstLine.textLength()
        ).trimmed();
        if (!secondLine.isValid()) {
            title->setText(first);
            updateHeight(false);
            return;
        }

        const int secondStart = secondLine.textStart();
        const int secondEnd = secondStart + secondLine.textLength();
        QString second = fullTitle.mid(secondStart, secondLine.textLength()).trimmed();
        if (secondEnd < fullTitle.size()) {
            second = title->fontMetrics().elidedText(
                fullTitle.mid(secondStart).trimmed(), Qt::ElideRight, width
            );
        }
        title->setText(first + "\n" + second);
        updateHeight(true);
    }

    void updateHeight(const bool twoLines) {
        const int lineHeight = title->fontMetrics().lineSpacing();
        title->setFixedHeight(lineHeight * (twoLines ? 2 : 1) + 2);
        const int cardHeight = twoLines ? 76 : 58;
        if (height() != cardHeight) {
            setFixedHeight(cardHeight);
            heightChanged(cardHeight);
        }
    }

    void applyStyle() {
        QFont font = title->font();
        font.setBold(unread);
        title->setFont(font);
        if (selected && unread) {
            setStyleSheet(
                "QFrame#rssArticleCard { background: #343434; border: none; "
                "border-left: 2px solid #7FA7E2; }"
            );
        } else if (selected) {
            setStyleSheet(
                "QFrame#rssArticleCard { background: #343434; border: none; "
                "border-left: 2px solid transparent; }"
            );
        } else if (unread) {
            setStyleSheet(
                "QFrame#rssArticleCard { background: rgba(127, 167, 226, 0.09); "
                "border: none; "
                "border-left: 2px solid #7FA7E2; }"
                "QFrame#rssArticleCard:hover { "
                "background: rgba(127, 167, 226, 0.14); }"
            );
        } else {
            setStyleSheet(
                "QFrame#rssArticleCard { background: transparent; border: none; "
                "border-left: 2px solid transparent; }"
                "QFrame#rssArticleCard:hover { "
                "background: rgba(255, 255, 255, 0.04); }"
            );
        }
    }

    QString fullTitle;
    QLabel *title;
    bool unread;
    bool selected = false;
    std::function<void(int)> heightChanged;
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
        layout->setContentsMargins(0, 7, 2, 5);
        layout->setSpacing(6);
        button = new QToolButton(this);
        button->setArrowType(collapsed ? Qt::RightArrow : Qt::DownArrow);
        button->setAutoRaise(true);
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

static QIcon categoryIcon(const RssCategory &category) {
    if (!category.icon.has_value()) {
        return {};
    }
    QPixmap pixmap;
    pixmap.loadFromData(*category.icon, "PNG");
    return QIcon(pixmap);
}
}

RssView::RssView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *navigationPane = new QWidget(this);
    navigationPane->setObjectName("rssNavigationPane");
    navigationPane->setFixedWidth(320);
    navigationPane->setStyleSheet(
        "QWidget#rssNavigationPane { border-right: 1px solid #383838; }"
    );
    auto *navigationLayout = new QVBoxLayout(navigationPane);
    navigationLayout->setContentsMargins(10, 12, 10, 12);
    navigationLayout->setSpacing(8);
    auto *navigationTitle = new QLabel("Feeds", navigationPane);
    QFont navigationTitleFont = navigationTitle->font();
    navigationTitleFont.setBold(true);
    navigationTitle->setFont(navigationTitleFont);
    navigationLayout->addWidget(navigationTitle);

    navigationTree = new QTreeWidget(navigationPane);
    navigationTree->setHeaderHidden(true);
    navigationTree->setRootIsDecorated(true);
    navigationTree->setTextElideMode(Qt::ElideRight);
    navigationTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigationTree->setContextMenuPolicy(Qt::CustomContextMenu);
    navigationTree->setIndentation(16);
    navigationTree->setStyleSheet(
        "QTreeWidget { background: transparent; border: none; }"
        "QTreeWidget::item { min-height: 25px; }"
    );
    navigationLayout->addWidget(navigationTree, 1);
    layout->addWidget(navigationPane);

    auto *articlePane = new QWidget(this);
    articlePane->setObjectName("rssArticlePane");
    articlePane->setFixedWidth(440);
    articlePane->setStyleSheet(
        "QWidget#rssArticlePane { border-right: 1px solid #383838; }"
    );
    auto *articleLayout = new QVBoxLayout(articlePane);
    articleLayout->setContentsMargins(10, 12, 10, 12);
    articleLayout->setSpacing(8);
    auto *articleTitle = new QLabel("Beiträge", articlePane);
    QFont articleTitleFont = articleTitle->font();
    articleTitleFont.setBold(true);
    articleTitle->setFont(articleTitleFont);
    articleLayout->addWidget(articleTitle);
    articleList = new QListWidget(articlePane);
    articleList->setFrameShape(QFrame::NoFrame);
    articleList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    articleList->setSpacing(2);
    articleList->setStyleSheet(
        "QListWidget { background: transparent; border: none; }"
        "QListWidget::item { border: none; background: transparent; }"
    );
    articleLayout->addWidget(articleList, 1);

    auto *detailPane = new QWidget(this);
    auto *detailLayout = new QVBoxLayout(detailPane);
    detailLayout->setContentsMargins(20, 18, 20, 18);
    detailLayout->setSpacing(8);
    detailTitle = new QLabel(detailPane);
    QFont detailTitleFont = detailTitle->font();
    detailTitleFont.setBold(true);
    detailTitleFont.setPointSize(detailTitleFont.pointSize() + 3);
    detailTitle->setFont(detailTitleFont);
    detailTitle->setWordWrap(true);
    detailLayout->addWidget(detailTitle);
    detailSource = new QLabel(detailPane);
    detailSource->setStyleSheet("color: #B8B8B8;");
    detailLayout->addWidget(detailSource);
    detailDate = new QLabel(detailPane);
    detailDate->setStyleSheet("color: #888888;");
    detailLayout->addWidget(detailDate);
    detailUrl = new QLabel(detailPane);
    detailUrl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailUrl->setStyleSheet("color: #8AB4F8;");
    detailUrl->setWordWrap(true);
    detailLayout->addWidget(detailUrl);
    detailSummary = new QTextBrowser(detailPane);
    detailSummary->setOpenExternalLinks(false);
    detailSummary->setFrameShape(QFrame::NoFrame);
    detailLayout->addWidget(detailSummary, 1);
    auto *detailButtons = new QHBoxLayout();
    openButton = new QPushButton("Im Browser öffnen", detailPane);
    openButton->setIcon(QIcon(":/assets/icons/open.png"));
    readButton = new QPushButton(detailPane);
    detailButtons->addWidget(openButton);
    detailButtons->addWidget(readButton);
    detailButtons->addStretch();
    detailLayout->addLayout(detailButtons);

    layout->addWidget(articlePane);
    layout->addWidget(detailPane, 1);

    connect(navigationTree, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem *item) {
                if (item) {
                    selectedNavigationKind = item->data(0, NAV_KIND_ROLE).toInt();
                    selectedNavigationId = item->data(0, NAV_ID_ROLE).toInt();
                }
                renderArticles();
            });
    connect(navigationTree, &QTreeWidget::customContextMenuRequested,
            this, &RssView::showNavigationContextMenu);
    connect(articleList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current, QListWidgetItem *previous) {
        updateCardSelection(current, previous);
        renderDetail();
    });
    connect(articleList, &QListWidget::itemDoubleClicked,
            this, [this] { openSelectedEntry(); });
    connect(openButton, &QPushButton::clicked, this, &RssView::openSelectedEntry);
    connect(readButton, &QPushButton::clicked,
            this, &RssView::toggleSelectedEntryRead);

    connect(&apiClient, &ApiClient::rssFeedsReceived, this,
            [this](const std::vector<RssFeed> &received) {
                feeds = received;
                renderNavigation();
            });
    connect(&apiClient, &ApiClient::rssCategoriesReceived, this,
            [this](const std::vector<RssCategory> &received) {
                categories = received;
                renderNavigation();
            });
    connect(&apiClient, &ApiClient::rssEntriesReceived, this,
            [this](const std::vector<RssEntry> &received) {
                entries = received;
                renderNavigation();
            });

    renderDetail();
}

void RssView::refresh() {
    apiClient.getRssCategories();
    apiClient.getRssFeeds();
    apiClient.getRssEntries();
}

int RssView::unreadCount(const std::optional<int> feedId) const {
    return static_cast<int>(std::count_if(
        entries.begin(), entries.end(), [feedId](const RssEntry &entry) {
            return !entry.isRead && (!feedId.has_value() || entry.feedId == *feedId);
        }
    ));
}

void RssView::renderNavigation() {
    const int selectedKind = selectedNavigationKind;
    const int selectedId = selectedNavigationId;
    const QSignalBlocker blocker(navigationTree);
    navigationTree->clear();

    auto *allItem = new QTreeWidgetItem(navigationTree, {
        "Alle Feeds (" + QString::number(unreadCount()) + ")"
    });
    allItem->setData(0, NAV_KIND_ROLE, AllFeeds);
    allItem->setData(0, NAV_ID_ROLE, 0);
    QFont allFont = allItem->font(0);
    allFont.setBold(true);
    allItem->setFont(0, allFont);
    QTreeWidgetItem *selection = selectedKind == AllFeeds ? allItem : nullptr;

    for (const RssCategory &category : categories) {
        auto *categoryItem = new QTreeWidgetItem(navigationTree, {category.name});
        categoryItem->setData(0, NAV_KIND_ROLE, Category);
        categoryItem->setData(0, NAV_ID_ROLE, category.id);
        categoryItem->setForeground(0, QColor(category.color));
        categoryItem->setIcon(0, categoryIcon(category));
        QFont categoryFont = categoryItem->font(0);
        categoryFont.setBold(true);
        categoryItem->setFont(0, categoryFont);
        categoryItem->setExpanded(true);
        if (selectedKind == Category && selectedId == category.id) {
            selection = categoryItem;
        }

        for (const RssFeed &feed : feeds) {
            if (feed.categoryId != category.id) {
                continue;
            }
            QString title = feed.title + " (" + QString::number(unreadCount(feed.id)) + ")";
            if (!feed.enabled) {
                title += " – deaktiviert";
            }
            auto *feedItem = new QTreeWidgetItem(categoryItem, {title});
            feedItem->setData(0, NAV_KIND_ROLE, Feed);
            feedItem->setData(0, NAV_ID_ROLE, feed.id);
            feedItem->setForeground(0, QColor(category.color));
            if (selectedKind == Feed && selectedId == feed.id) {
                selection = feedItem;
            }
        }
    }

    auto *unassignedItem = new QTreeWidgetItem(navigationTree, {"Unzugeordnet"});
    unassignedItem->setData(0, NAV_KIND_ROLE, Unassigned);
    unassignedItem->setData(0, NAV_ID_ROLE, 0);
    QFont unassignedFont = unassignedItem->font(0);
    unassignedFont.setBold(true);
    unassignedItem->setFont(0, unassignedFont);
    unassignedItem->setExpanded(true);
    if (selectedKind == Unassigned) {
        selection = unassignedItem;
    }
    for (const RssFeed &feed : feeds) {
        if (feed.categoryId.has_value()) {
            continue;
        }
        QString title = feed.title + " (" + QString::number(unreadCount(feed.id)) + ")";
        if (!feed.enabled) {
            title += " – deaktiviert";
        }
        auto *feedItem = new QTreeWidgetItem(unassignedItem, {title});
        feedItem->setData(0, NAV_KIND_ROLE, Feed);
        feedItem->setData(0, NAV_ID_ROLE, feed.id);
        if (selectedKind == Feed && selectedId == feed.id) {
            selection = feedItem;
        }
    }

    navigationTree->setCurrentItem(selection ? selection : allItem);
    renderArticles();
}

void RssView::renderArticles() {
    const int selectedEntryId = selectedEntry() ? selectedEntry()->id : 0;
    const int kind = navigationTree->currentItem()
        ? navigationTree->currentItem()->data(0, NAV_KIND_ROLE).toInt()
        : AllFeeds;
    const int id = navigationTree->currentItem()
        ? navigationTree->currentItem()->data(0, NAV_ID_ROLE).toInt()
        : 0;
    const QSignalBlocker blocker(articleList);
    articleList->clear();
    QListWidgetItem *selection = nullptr;
    QListWidgetItem *firstVisibleEntry = nullptr;
    QString currentGroupKey;
    const QLocale german(QLocale::German, QLocale::Germany);

    for (const RssEntry &entry : entries) {
        RssFeed *feed = feedById(entry.feedId);
        if (!feed) {
            continue;
        }
        const bool visible = kind == AllFeeds
            || (kind == Feed && feed->id == id)
            || (kind == Category && feed->categoryId == id)
            || (kind == Unassigned && !feed->categoryId.has_value());
        if (!visible) {
            continue;
        }

        const QDate publishedDate = entry.publishedAt.has_value()
            ? entry.publishedAt->toLocalTime().date()
            : QDate();
        const QString groupKey = publishedDate.isValid()
            ? publishedDate.toString(Qt::ISODate)
            : QString("without-date");
        if (groupKey != currentGroupKey) {
            currentGroupKey = groupKey;
            const QString groupTitle = publishedDate.isValid()
                ? german.toString(publishedDate, "dddd, d. MMMM")
                : QString("Ohne Datum");
            auto *headerItem = new QListWidgetItem(articleList);
            headerItem->setFlags(headerItem->flags() & ~Qt::ItemIsSelectable);
            headerItem->setSizeHint(QSize(360, 36));
            articleList->setItemWidget(headerItem, new DateHeader(
                groupTitle,
                collapsedArticleGroups.contains(groupKey),
                [this, groupKey](const bool collapsed) {
                    if (collapsed) {
                        collapsedArticleGroups.insert(groupKey);
                    } else {
                        collapsedArticleGroups.remove(groupKey);
                    }
                    for (int index = 0; index < articleList->count(); ++index) {
                        QListWidgetItem *groupItem = articleList->item(index);
                        if (groupItem->data(GROUP_KEY_ROLE).toString() == groupKey) {
                            groupItem->setHidden(collapsed);
                        }
                    }
                },
                articleList
            ));
        }

        auto *item = new QListWidgetItem(articleList);
        item->setData(ENTRY_ID_ROLE, entry.id);
        item->setData(GROUP_KEY_ROLE, groupKey);
        item->setSizeHint(QSize(360, 76));
        articleList->setItemWidget(item, new ArticleCard(
            entry,
            [item](const int height) {
                item->setSizeHint(QSize(item->sizeHint().width(), height));
            },
            articleList
        ));
        const bool collapsed = collapsedArticleGroups.contains(groupKey);
        item->setHidden(collapsed);
        if (!collapsed && !firstVisibleEntry) {
            firstVisibleEntry = item;
        }
        if (!collapsed && entry.id == selectedEntryId) {
            selection = item;
        }
    }
    if (!selection) {
        selection = firstVisibleEntry;
    }
    articleList->setCurrentItem(selection);
    updateCardSelection(selection, nullptr);
    renderDetail();
}

void RssView::updateCardSelection(
    QListWidgetItem *current,
    QListWidgetItem *previous
) {
    if (previous && previous->data(ENTRY_ID_ROLE).isValid()) {
        auto *card = static_cast<ArticleCard *>(articleList->itemWidget(previous));
        card->setSelected(false);
    }
    if (current && current->data(ENTRY_ID_ROLE).isValid()) {
        auto *card = static_cast<ArticleCard *>(articleList->itemWidget(current));
        card->setSelected(true);
    }
}

RssEntry *RssView::selectedEntry() {
    if (!articleList->currentItem()) {
        return nullptr;
    }
    const int id = articleList->currentItem()->data(ENTRY_ID_ROLE).toInt();
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

RssCategory *RssView::categoryById(const int id) {
    const auto found = std::find_if(categories.begin(), categories.end(),
                                    [id](const RssCategory &category) {
                                        return category.id == id;
                                    });
    return found == categories.end() ? nullptr : &*found;
}

void RssView::renderDetail() {
    RssEntry *entry = selectedEntry();
    const bool hasEntry = entry != nullptr;
    detailTitle->setText(hasEntry ? entry->title : "Kein Beitrag ausgewählt");
    detailSource->setText(hasEntry ? entry->feedTitle : QString());
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
    form->addRow(buttons);
    prepareDialog(dialog);
    if (execDialog(dialog) != QDialog::Accepted || urlEdit->text().trimmed().isEmpty()) {
        return;
    }
    apiClient.createRssFeed(urlEdit->text().trimmed());
}

void RssView::openCreateCategoryDialog() {
    openCategoryDialog(nullptr);
}

void RssView::openCategoryDialog(const RssCategory *category) {
    QDialog dialog(this);
    dialog.setWindowTitle(category ? "Kategorie bearbeiten" : "Kategorie erstellen");
    auto *form = new QFormLayout(&dialog);
    auto *nameEdit = new QLineEdit(category ? category->name : QString(), &dialog);
    form->addRow("Name", nameEdit);

    std::optional<QByteArray> icon = category ? category->icon : std::nullopt;
    auto *iconButton = new QPushButton(&dialog);
    const auto updateIconButton = [&] {
        iconButton->setText(icon.has_value() ? "Icon ändern" : "Icon auswählen");
        if (icon.has_value()) {
            QPixmap pixmap;
            pixmap.loadFromData(*icon, "PNG");
            iconButton->setIcon(QIcon(pixmap));
        }
    };
    updateIconButton();
    connect(iconButton, &QPushButton::clicked, &dialog, [&] {
        const QString fileName = QFileDialog::getOpenFileName(
            &dialog, "Icon auswählen", QString(), "PNG-Bilder (*.png)"
        );
        if (fileName.isEmpty()) {
            return;
        }
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            icon = file.readAll();
            updateIconButton();
        }
    });
    form->addRow("Icon", iconButton);

    QColor color(category ? category->color : "#E5E5E5");
    auto *colorButton = new QPushButton(&dialog);
    const auto updateColorButton = [&] {
        colorButton->setText(color.name().toUpper());
        colorButton->setStyleSheet("color: " + color.name() + ";");
    };
    updateColorButton();
    connect(colorButton, &QPushButton::clicked, &dialog, [&] {
        const QColor selected = QColorDialog::getColor(color, &dialog, "Textfarbe");
        if (selected.isValid()) {
            color = selected;
            updateColorButton();
        }
    });
    form->addRow("Textfarbe", colorButton);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog
    );
    buttons->button(QDialogButtonBox::Save)->setText(
        category ? "Speichern" : "Erstellen"
    );
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);
    prepareDialog(dialog);
    if (execDialog(dialog) != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return;
    }
    if (category) {
        apiClient.updateRssCategory(
            category->id, nameEdit->text().trimmed(), icon, color.name()
        );
    } else {
        apiClient.createRssCategory(nameEdit->text().trimmed(), icon, color.name());
    }
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
    form->addRow(buttons);
    prepareDialog(dialog);
    if (execDialog(dialog) != QDialog::Accepted || urlEdit->text().trimmed().isEmpty()) {
        return;
    }
    apiClient.updateRssFeed(feed.id, urlEdit->text().trimmed(), enabledEdit->isChecked());
}

void RssView::showNavigationContextMenu(const QPoint &position) {
    QTreeWidgetItem *item = navigationTree->itemAt(position);
    if (!item) {
        return;
    }
    const int kind = item->data(0, NAV_KIND_ROLE).toInt();
    const int id = item->data(0, NAV_ID_ROLE).toInt();

    if (kind == Category) {
        RssCategory *category = categoryById(id);
        if (!category) {
            return;
        }
        const RssCategory selectedCategory = *category;
        QMenu menu(this);
        QAction *editAction = menu.addAction("Bearbeiten");
        QAction *deleteAction = menu.addAction("Löschen");
        QAction *selected = menu.exec(navigationTree->viewport()->mapToGlobal(position));
        if (selected == editAction) {
            openCategoryDialog(&selectedCategory);
        } else if (selected == deleteAction) {
            apiClient.deleteRssCategory(selectedCategory.id);
        }
        return;
    }

    if (kind != Feed) {
        return;
    }
    RssFeed *feed = feedById(id);
    if (!feed) {
        return;
    }
    const RssFeed selectedFeed = *feed;

    QMenu menu(this);
    QAction *refreshAction = menu.addAction("Aktualisieren");
    refreshAction->setEnabled(selectedFeed.enabled);
    QMenu *categoryMenu = menu.addMenu("Kategorie zuweisen");
    QAction *unassignedAction = categoryMenu->addAction("Unzugeordnet");
    std::vector<std::pair<QAction *, int>> categoryActions;
    for (const RssCategory &category : categories) {
        categoryActions.emplace_back(categoryMenu->addAction(category.name), category.id);
    }
    QAction *readAction = menu.addAction("Als gelesen markieren");
    QAction *editAction = menu.addAction("Bearbeiten");
    menu.addSeparator();
    QAction *hideAction = menu.addAction("Ausblenden");
    QAction *selected = menu.exec(navigationTree->viewport()->mapToGlobal(position));
    if (selected == refreshAction) {
        apiClient.refreshRssFeed(selectedFeed.id);
    } else if (selected == unassignedAction) {
        apiClient.assignRssFeedCategory(selectedFeed.id, std::nullopt);
    } else if (selected == readAction) {
        apiClient.markRssFeedRead(selectedFeed.id);
    } else if (selected == editAction) {
        openFeedDialog(selectedFeed);
    } else if (selected == hideAction) {
        apiClient.deleteRssFeed(selectedFeed.id);
    } else {
        const auto categoryAction = std::find_if(
            categoryActions.begin(), categoryActions.end(),
            [selected](const auto &entry) { return entry.first == selected; }
        );
        if (categoryAction != categoryActions.end()) {
            apiClient.assignRssFeedCategory(selectedFeed.id, categoryAction->second);
        }
    }
}
