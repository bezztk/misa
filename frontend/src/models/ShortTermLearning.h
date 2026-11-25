#ifndef FRONTEND_SHORTTERMLEARNING_H
#define FRONTEND_SHORTTERMLEARNING_H

#include <QDateTime>
#include <QString>

#include <array>
#include <optional>
#include <vector>

enum class ShortTermLearningState {
    New = 1,
    Learning = 2,
    Completed = 3,
};

enum class ShortTermLearningRating {
    Again = 1,
    Hard = 2,
    Good = 3,
    Easy = 4,
};

struct ShortTermLearningProgress {
    ShortTermLearningState state;
    std::optional<ShortTermLearningRating> lastRating;
    QDateTime dueAt;
};

struct ShortTermLearningSet {
    int id;
    QString title;
    bool bidirectional;
    qint64 cardCount;
    qint64 newCount;
    qint64 learningCount;
    qint64 completedCount;
};

struct ShortTermLearningCard {
    int id;
    QString front;
    QString hint;
    QString back;
    ShortTermLearningProgress frontToBack;
    std::optional<ShortTermLearningProgress> backToFront;
};

struct ShortTermLearningRatingOption {
    ShortTermLearningRating rating;
    QString displayName;
    std::optional<int> intervalMinutes;
};

struct ShortTermLearningCards {
    std::vector<ShortTermLearningCard> cards;
    std::array<ShortTermLearningRatingOption, 4> ratings;
};

#endif // FRONTEND_SHORTTERMLEARNING_H
