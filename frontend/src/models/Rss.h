#ifndef FRONTEND_RSS_H
#define FRONTEND_RSS_H

#include <QDateTime>
#include <QByteArray>
#include <QString>

#include <optional>

struct RssFeed {
    int id;
    std::optional<int> categoryId;
    QString url;
    QString title;
    std::optional<QDateTime> lastFetchedAt;
    bool enabled;
    bool isHidden;
};

struct RssCategory {
    int id;
    QString name;
    std::optional<QByteArray> icon;
    QString color;
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
