#ifndef FRONTEND_MEDIA_H
#define FRONTEND_MEDIA_H

#include <QDate>
#include <QDateTime>
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
    QString productionStatus;
    QStringList keywords;
    QString posterUrl;
    QString backdropUrl;
    QString trailerUrl;
    QStringList additionalBackdropPaths;
    int seasonCount;
    int episodeCount;
    int watchCount;
    QDateTime completedAt;
    QDateTime lastSyncedAt;
    QString personalNotes;

    QString displayTitle() const {
        if (!title.isEmpty()) {
            return title;
        }
        return originalTitle.isEmpty() ? QStringLiteral("Ohne Titel") : originalTitle;
    }
};

struct MediaSeason {
    int id;
    int number;
    QString title;
    QString description;
    QString posterPath;
    QDate releaseDate;
    int episodeCount;
    int completedEpisodeCount;
};

struct MediaEpisode {
    int id;
    int number;
    QString title;
    QString description;
    QString stillPath;
    std::optional<int> runtimeMinutes;
    QDate releaseDate;
    int watchStatusId;
    QDateTime watchedAt;
};

struct MediaEpisodeProgress {
    int episodeCount;
    int completedEpisodeCount;
    double percentage;
};

struct MediaSeriesProgress {
    MediaEpisodeProgress regular;
    MediaEpisodeProgress specials;
};

#endif // FRONTEND_MEDIA_H
