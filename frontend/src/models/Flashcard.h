#ifndef FRONTEND_FLASHCARD_H
#define FRONTEND_FLASHCARD_H

#include <QDateTime>
#include <optional>

struct Flashcard {
    int id;
    QString front;
    QString back;
};

enum class FlashcardRating : int16_t {
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
    int16_t review_state_id;
    int16_t learning_step;
    int retention_class_id;
    std::optional<float> stability;
    std::optional<float> difficulty;
    FlashcardReviewPreview preview;
};

struct ReviewFlashcard {
    int16_t rating;
    int duration_ms;
};

#endif //FRONTEND_FLASHCARD_H
