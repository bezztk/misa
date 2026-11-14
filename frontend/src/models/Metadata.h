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

struct FlashcardRetentionClass {
    int id;
    QString name;
    std::optional<QString> display_name;
    double desired_retention;
};

struct SystemTaskSessionPreset {
    int id;
    QString name;
    int planned_duration_minutes;
};

struct Metadata {
    std::vector<MetadataItem> personal_relevance_levels;
    std::vector<MetadataItem> lasting_impacts;
    std::vector<MetadataItem> cognitive_levels;
    std::vector<MetadataItem> psychomotor_levels;
    std::vector<MetadataItem> document_completeness_levels;
    std::vector<MetadataItem> document_flashcard_statuses;
    std::vector<MetadataItem> document_verification_statuses;
    std::vector<MetadataItem> document_verification_levels;
    std::vector<MetadataItem> domains;
    std::vector<FlashcardRetentionClass> flashcard_retention_classes;
    std::vector<MetadataItem> flashcard_review_states;
    std::vector<MetadataItem> pursuit_requirement_types;
    std::vector<MetadataItem> systemtask_groups;
    std::vector<MetadataItem> systemtask_states;
    std::vector<MetadataItem> systemtask_priorities;
    std::vector<MetadataItem> systemtask_routine_execution_states;
    std::vector<SystemTaskSessionPreset> systemtask_session_presets;
    std::vector<MetadataItem> systemtask_session_states;
};

#endif //FRONTEND_METADATA_H
