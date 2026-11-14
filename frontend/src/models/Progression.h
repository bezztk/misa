#ifndef FRONTEND_PROGRESSION_H
#define FRONTEND_PROGRESSION_H

#include <QString>

#include <optional>
#include <vector>

struct ProgressionPursuit {
    int id;
    QString name;
    std::optional<int> progressionStageId;
};

struct ProgressionStage {
    int id;
    int stage;
    QString name;
    std::vector<ProgressionPursuit> pursuits;
};

struct Progression {
    int id;
    QString name;
    std::optional<int> personalRelevanceId;
    std::optional<int> lastingImpactId;
    std::vector<ProgressionStage> stages;
};

#endif // FRONTEND_PROGRESSION_H
