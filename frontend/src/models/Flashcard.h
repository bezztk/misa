#ifndef FRONTEND_FLASHCARD_H
#define FRONTEND_FLASHCARD_H

#include <QString>

#include <cstdint>
#include <optional>
#include <vector>

#include "Comment.h"

enum class FlashcardField {
    Front = 1,
    Back = 2
};

struct Flashcard {
    int id;
    QString front;
    QString back;
    std::vector<Comment> comments;
};

enum class FlashcardRating : std::int16_t {
    Again = 1,
    Hard = 2,
    Good = 3,
    Easy = 4
};

struct FlashcardReviewPreview {
    float again;
    float hard;
    float good;
    float easy;
};

struct FlashcardReviewQueueItem {
    int id;
    QString front;
    QString back;
    QString hint;
    std::int16_t reviewStateId;
    std::int16_t learningStep;
    int retentionClassId;
    std::optional<float> stability;
    std::optional<float> difficulty;
    FlashcardReviewPreview preview;
    std::vector<Comment> comments;
};

struct ReviewFlashcard {
    std::int16_t rating;
    int durationMs;
};

#endif //FRONTEND_FLASHCARD_H
