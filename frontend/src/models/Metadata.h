#ifndef FRONTEND_METADATA_H
#define FRONTEND_METADATA_H

#include <optional>
#include <QString>
#include <vector>

struct MetadataItem {
    int id;
    QString name;
    std::optional<QString> displayName{};
};

struct FlashcardRetentionClass {
    int id;
    QString name;
    std::optional<QString> displayName{};
    double desiredRetention;
};

struct Metadata {
    std::vector<MetadataItem> personalRelevanceLevels;
    std::vector<MetadataItem> lastingImpacts;
    std::vector<MetadataItem> cognitiveLevels;
    std::vector<MetadataItem> psychomotorLevels;
    std::vector<MetadataItem> documentCompletenessLevels;
    std::vector<MetadataItem> documentFlashcardStatuses;
    std::vector<MetadataItem> documentVerificationStatuses;
    std::vector<MetadataItem> documentVerificationLevels;
    std::vector<FlashcardRetentionClass> flashcardRetentionClasses;
    std::vector<MetadataItem> systemTaskGroups;
    std::vector<MetadataItem> systemTaskStates;
    std::vector<MetadataItem> systemTaskPriorities;
};

#endif //FRONTEND_METADATA_H
