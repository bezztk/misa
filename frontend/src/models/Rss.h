#ifndef FRONTEND_RSS_H
#define FRONTEND_RSS_H

#include <QDateTime>
#include <QString>

#include <optional>

struct RssFeed {
    int id;
    QString url;
    QString title;
    std::optional<QDateTime> lastFetchedAt;
    bool enabled;
    bool isHidden;
};

struct RssEntry {
    int id;
    int feedId;
    QString feedTitle;
    QString externalId;
    QString title;
    QString url;
    QString summary;
    std::optional<QDateTime> publishedAt;
    bool isRead;
};

#endif // FRONTEND_RSS_H
