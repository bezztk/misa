#ifndef FRONTEND_APICLIENT_H
#define FRONTEND_APICLIENT_H
#include "../models/Task.h"
#include "../models/Flashcard.h"
#include "../models/Metadata.h"
#include "../models/DomainNode.h"
#include <QNetworkAccessManager>

class ApiClient : public QObject {
    Q_OBJECT

signals:
    void metadataReceived();
    void taskReceived(const std::vector<Task>& tasks);
    void taskCreated();
    void taskUpdated();
    void taskDeleted();
    void flashcardsReceived(const std::vector<FlashcardReviewQueueItem>& flashcards);
    void flashcardRated();
    void flashcardPreviewCalculated(const FlashcardReviewPreview& preview);
    void domainNodesReceived(const std::vector<DomainNode>& nodes);
    void domainNodeCreated(int parentId, const DomainNode& node);
    void domainNodeUpdated(const DomainNode& node);

public:
    ApiClient();
    void getMetadata();
    void getTasks();
    void createTask(const CreateTask& task);
    void updateTask(int id, const UpdateTask& task);
    void deleteTask(int id);

    void getDueFlashcards();
    void rateFlashcard(int id, ReviewFlashcard reviewFlashcard);
    void getFlashcardPreview(int id);
    void getDomainNodes();
    void createDomainNode(int parentId, const QString& name);
    void updateDomainNode(int id, const QString& name);

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
