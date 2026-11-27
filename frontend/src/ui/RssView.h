#ifndef FRONTEND_RSSVIEW_H
#define FRONTEND_RSSVIEW_H

#include "../models/Rss.h"

#include <QWidget>
#include <QSet>

#include <vector>

class ApiClient;
class QListWidget;
class QListWidgetItem;
class QPoint;

class RssView final : public QWidget {
public:
    explicit RssView(ApiClient &apiClient, QWidget *parent = nullptr);
    void refresh();
    void openCreateFeedDialog();

private:
    void renderNavigation();
    void updateUnreadCounts();
    void renderArticles();
    void toggleArticle(QListWidgetItem *item);
    void openFeedDialog(const RssFeed &feed);
    void showNavigationContextMenu(const QPoint &position);

    ApiClient &apiClient;
    std::vector<RssFeed> feeds;
    std::vector<RssEntry> entries;
    QListWidget *navigation;
    QListWidget *articleList;
    QSet<QString> collapsedArticleGroups;
    QSet<int> expandedArticles;
};

#endif // FRONTEND_RSSVIEW_H
