#ifndef FRONTEND_METADATA_H
#define FRONTEND_METADATA_H

#include <optional>
#include <QString>
#include <vector>

struct MetadataItem {
    int id;
    QString name;
    std::optional<QString> display_name;
};

struct TaskGroupMetadata {
    int id;
    QString name;
};

struct FlashcardRetentionLevel {
    int id;
    QString name;
    std::optional<QString> display_name;
    double desired_retention;
};

struct Metadata {
    std::vector<TaskGroupMetadata> task_groups;
    std::vector<MetadataItem> task_states;
    std::vector<MetadataItem> task_priorities;
    std::vector<MetadataItem> topic_understanding_levels;
    std::vector<MetadataItem> document_completeness_levels;
    std::vector<MetadataItem> document_flashcard_statuses;
    std::vector<MetadataItem> document_verification_statuses;
    std::vector<MetadataItem> document_verification_levels;
    std::vector<FlashcardRetentionLevel> flashcard_retention_levels;
};

class AppState {
public:
    inline static Metadata metadata;
    inline static bool metadataLoaded = false;
};

#endif //FRONTEND_METADATA_H
