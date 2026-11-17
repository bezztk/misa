#ifndef FRONTEND_FLASHCARD_H
#define FRONTEND_FLASHCARD_H

#include <QString>

#include <cstdint>
#include <optional>

struct Flashcard {
    int id;
    QString front;
    QString back;
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
    std::int16_t reviewStateId;
    std::int16_t learningStep;
    int retentionClassId;
    std::optional<float> stability;
    std::optional<float> difficulty;
    FlashcardReviewPreview preview;
};

struct ReviewFlashcard {
    std::int16_t rating;
    int durationMs;
};

#endif //FRONTEND_FLASHCARD_H
