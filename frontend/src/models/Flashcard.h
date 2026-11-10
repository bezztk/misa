#ifndef FRONTEND_FLASHCARD_H
#define FRONTEND_FLASHCARD_H

#include <QDateTime>
#include <optional>

enum class FlashcardRating : int16_t {
    Again = 1,
    Hard = 2,
    Good = 3,
    Easy = 4
};

struct FlashcardReviewQueueItem {
    int id;
    QString front;
    QString back;
    int16_t state;
    int16_t learning_step;
    int retention_levels_id;
    std::optional<float> stability;
    std::optional<float> difficulty;
};

struct ReviewFlashcard {
    int16_t rating;
    int duration_ms;
};

struct FlashcardReviewPreview {
    float again;
    float hard;
    float good;
    float easy;
};

#endif //FRONTEND_FLASHCARD_H
