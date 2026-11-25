#ifndef FRONTEND_MEDIA_H
#define FRONTEND_MEDIA_H

#include <QDate>
#include <QString>
#include <QStringList>

#include <optional>

enum class MediaRating {
    Disappointing = 1,
    Average,
    WorthWatching,
    Outstanding
};

struct Media {
    int entityId;
    int tmdbId;
    QString mediaType;
    QString title;
    QString originalTitle;
    QDate releaseDate;
    QString posterPath;
    int watchStatusId;
    std::optional<MediaRating> personalRating;
    QString backdropPath;
    QString description;
    QStringList genres;
    std::optional<int> runtimeMinutes;
    std::optional<double> tmdbVoteAverage;
    std::optional<int> tmdbVoteCount;

    QString displayTitle() const {
        if (!title.isEmpty()) {
            return title;
        }
        return originalTitle.isEmpty() ? QStringLiteral("Ohne Titel") : originalTitle;
    }
};

#endif // FRONTEND_MEDIA_H
