#ifndef FRONTEND_MEDIA_H
#define FRONTEND_MEDIA_H

#include <QDate>
#include <QString>

#include <optional>

struct Media {
    int entityId;
    int tmdbId;
    QString mediaType;
    QString title;
    QString originalTitle;
    QDate releaseDate;
    QString posterPath;
    int watchStatusId;
    std::optional<int> personalRating;

    QString displayTitle() const {
        if (!title.isEmpty()) {
            return title;
        }
        return originalTitle.isEmpty() ? QStringLiteral("Ohne Titel") : originalTitle;
    }
};

#endif // FRONTEND_MEDIA_H
