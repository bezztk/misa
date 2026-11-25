#ifndef FRONTEND_SHORTTERMLEARNING_H
#define FRONTEND_SHORTTERMLEARNING_H

#include <QString>

#include <optional>

enum class ShortTermLearningState {
    New = 1,
    Learning = 2,
    Completed = 3,
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
    ShortTermLearningState frontToBackState;
    std::optional<ShortTermLearningState> backToFrontState;
};

#endif // FRONTEND_SHORTTERMLEARNING_H
