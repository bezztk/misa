#ifndef FRONTEND_RSSVIEW_H
#define FRONTEND_RSSVIEW_H

#include "../api/ApiClient.h"

#include <QWidget>

#include <vector>

class QLabel;
class QLineEdit;
class QListWidget;
class QPoint;
class QPushButton;
class QTextBrowser;
class QTreeWidget;

class RssView final : public QWidget {
public:
    explicit RssView(ApiClient &apiClient, QWidget *parent = nullptr);
    void refresh();

private:
    void renderFeeds();
    void renderEntries();
    void renderDetail();
    void subscribe();
    void showFeedContextMenu(const QPoint &position);
    void showEntryContextMenu(const QPoint &position);
    void openSelectedEntry();
    void toggleSelectedEntryRead();
    RssEntry *selectedEntry();
    RssFeed *feedById(int id);

    ApiClient &apiClient;
    std::vector<RssFeed> feeds;
    std::vector<RssEntry> entries;
    QLineEdit *urlEdit;
    QListWidget *feedList;
    QTreeWidget *entryList;
    QLabel *detailTitle;
    QLabel *detailUrl;
    QLabel *detailDate;
    QTextBrowser *detailSummary;
    QPushButton *openButton;
    QPushButton *readButton;
};

#endif // FRONTEND_RSSVIEW_H
