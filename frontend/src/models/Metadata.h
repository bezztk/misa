#ifndef FRONTEND_METADATA_H
#define FRONTEND_METADATA_H

#include <optional>
#include <QByteArray>
#include <QString>
#include <vector>

struct MetadataItem {
    int id;
    QString name;
    std::optional<QString> displayName{};
    std::optional<QString> description{};
    std::optional<QString> color{};
    std::optional<QByteArray> icon{};
};

struct KnowledgeLevel {
    int id;
    QString name;
    QString displayName;
    QString description;
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
    std::vector<KnowledgeLevel> knowledgeLevels;
    std::vector<MetadataItem> documentCompletenessLevels;
    std::vector<MetadataItem> documentFlashcardStatuses;
    std::vector<MetadataItem> documentVerificationStatuses;
    std::vector<MetadataItem> documentVerificationLevels;
    std::vector<FlashcardRetentionClass> flashcardRetentionClasses;
    std::vector<MetadataItem> ideaPhases;
    std::vector<MetadataItem> ideaStates;
    std::vector<MetadataItem> mediaRatings;
    std::vector<MetadataItem> systemTaskGroups;
    std::vector<MetadataItem> systemTaskStates;
    std::vector<MetadataItem> systemTaskPriorities;
};

#endif //FRONTEND_METADATA_H
