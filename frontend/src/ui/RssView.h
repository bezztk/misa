#ifndef FRONTEND_RSSVIEW_H
#define FRONTEND_RSSVIEW_H

#include "../api/ApiClient.h"

#include <QWidget>
#include <QSet>

#include <vector>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPoint;
class QPushButton;
class QTextBrowser;
class QTreeWidget;

class RssView final : public QWidget {
public:
    explicit RssView(ApiClient &apiClient, QWidget *parent = nullptr);
    void refresh();
    void openCreateFeedDialog();
    void openCreateCategoryDialog();

private:
    void renderNavigation();
    void renderArticles();
    void renderDetail();
    void updateCardSelection(QListWidgetItem *current, QListWidgetItem *previous);
    void openCategoryDialog(const RssCategory *category);
    void openFeedDialog(const RssFeed &feed);
    void showNavigationContextMenu(const QPoint &position);
    void openSelectedEntry();
    void toggleSelectedEntryRead();
    RssEntry *selectedEntry();
    RssFeed *feedById(int id);
    RssCategory *categoryById(int id);
    int unreadCount(std::optional<int> feedId = std::nullopt) const;

    ApiClient &apiClient;
    std::vector<RssFeed> feeds;
    std::vector<RssCategory> categories;
    std::vector<RssEntry> entries;
    QTreeWidget *navigationTree;
    QListWidget *articleList;
    QLabel *detailTitle;
    QLabel *detailSource;
    QLabel *detailUrl;
    QLabel *detailDate;
    QTextBrowser *detailSummary;
    QPushButton *openButton;
    QPushButton *readButton;
    int selectedNavigationKind = 0;
    int selectedNavigationId = 0;
    QSet<QString> collapsedArticleGroups;
};

#endif // FRONTEND_RSSVIEW_H
