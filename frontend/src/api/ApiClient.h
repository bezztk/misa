#ifndef FRONTEND_APICLIENT_H
#define FRONTEND_APICLIENT_H
#include "../models/Task.h"
#include "../models/Flashcard.h"
#include "../models/Metadata.h"
#include <QNetworkAccessManager>

class ApiClient : public QObject {
    Q_OBJECT

signals:
    void taskReceived(const std::vector<Task>& tasks);
    void flashcardsReceived(const std::vector<FlashcardReviewQueueItem>& flashcards);
    void flashcardRated();
    void flashcardPreviewCalculated(const FlashcardReviewPreview& preview);

public:
    ApiClient();
    void getMetadata();
    void getTasks();

    void getDueFlashcards();
    void rateFlashcard(int id, ReviewFlashcard reviewFlashcard);
    void getFlashcardPreview(int id);

private:
    static constexpr auto SCHEME = "http";
    static constexpr auto HOST = "localhost";
    static constexpr int PORT = 3000;
    static constexpr auto BASE_PATH = "/api";
    static constexpr auto PATH_TASKS = "/tasks";
    static constexpr auto PATH_METADATA = "/metadata";
    static constexpr auto PATH_FLASHCARDS = "/flashcards/due";

    static QUrl buildUrl(const QString& path);

    QNetworkAccessManager manager;
};

#endif //FRONTEND_APICLIENT_H
