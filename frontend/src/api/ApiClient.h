#ifndef FRONTEND_APICLIENT_H
#define FRONTEND_APICLIENT_H
#include "../models/Task.h"
#include "../models/Flashcard.h"
#include "../models/Metadata.h"
#include "../models/DomainNode.h"
#include "../models/Document.h"
#include "../models/Skill.h"
#include "../models/Pursuit.h"
#include "../models/Progression.h"
#include "../models/CalendarEntry.h"
#include <QNetworkAccessManager>

class ApiClient : public QObject {
    Q_OBJECT

signals:
    void metadataReceived();
    void taskReceived(const std::vector<Task>& tasks);
    void taskCreated();
    void taskUpdated();
    void taskDeleted();
    void calendarEntriesReceived(const std::vector<CalendarEntry>& entries);
    void calendarEntryCreated();
    void flashcardsReceived(const std::vector<FlashcardReviewQueueItem>& flashcards);
    void flashcardRated();
    void nodeFlashcardsReceived(int nodeId, const std::vector<Flashcard>& flashcards);
    void nodeFlashcardsChanged(int nodeId);
    void nodeSkillsReceived(int nodeId, const std::vector<Skill>& skills);
    void nodeSkillsChanged(int nodeId);
    void nodePursuitsReceived(int nodeId, const std::vector<Pursuit>& pursuits);
    void nodePursuitsChanged(int nodeId);
    void progressionsReceived(const std::vector<Progression>& progressions,
                              const std::vector<ProgressionPursuit>& pursuits);
    void availableProgressionPursuitsReceived(
        int stageId,
        const std::vector<ProgressionPursuit>& pursuits
    );
    void progressionsChanged();
    void domainTreeReceived(
        const std::vector<MetadataItem>& domains,
        const std::vector<DomainNode>& nodes
    );
    void domainNodeCreated(const DomainNode& node);
    void domainNodeUpdated(const DomainNode& node);
    void documentCreated(const Document& document);
    void documentReceived(const Document& document);
    void documentUpdated(bool successful);

public:
    ApiClient();
    void getMetadata();
    void getTasks();
    void createTask(const CreateTask& task);
    void updateTask(int id, const UpdateTask& task);
    void deleteTask(int id);
    void getCalendarEntries();
    void createCalendarEntry(const CreateCalendarEntry& entry);

    void getDueFlashcards();
    void rateFlashcard(int id, ReviewFlashcard reviewFlashcard);
    void getNodeFlashcards(int nodeId);
    void createFlashcard(int nodeId, const QString& front, const QString& back);
    void updateFlashcard(int nodeId, int id, const QString& front, const QString& back);
    void deleteFlashcard(int nodeId, int id);
    void getNodeSkills(int nodeId);
    void createSkill(int nodeId, std::optional<int> parentSkillId,
                     const QString& name, std::optional<int> psychomotorLevelId);
    void updateSkill(int nodeId, int id, const QString& name,
                     std::optional<int> psychomotorLevelId);
    void getNodePursuits(int nodeId);
    void createPursuit(int nodeId, const QString& name);
    void updatePursuitRequirements(int nodeId, int pursuitId,
                                   const UpdatePursuitRequirements& requirements);
    void getProgressions();
    void getAvailableProgressionPursuits(int stageId);
    void createProgression(const QString& name);
    void updateProgression(int id, const QString& name,
                           std::optional<int> personalRelevanceId,
                           std::optional<int> lastingImpactId);
    void createProgressionStage(int progressionId, const QString& name);
    void assignPursuitToStage(int stageId, int pursuitId);
    void getDomainTree();
    void createDomainNode(int parentId, const QString& name);
    void createRootDomainNode(int domainId, const QString& name);
    void updateDomainNode(int id, const QString& name,
                          std::optional<int> cognitiveLevelId,
                          std::optional<int> completenessLevelId,
                          std::optional<int> flashcardStatusId,
                          std::optional<int> verificationStatusId,
                          std::optional<int> verificationLevelId);
    void createDocument(int nodeId);
    void getDocument(int nodeId);
    void updateDocument(int nodeId, const QString& content);

private:
    static constexpr auto SCHEME = "http";
    static constexpr auto HOST = "localhost";
    static constexpr int PORT = 3000;
    static constexpr auto BASE_PATH = "/api";
    static constexpr auto PATH_TASKS = "/tasks";
    static constexpr auto PATH_METADATA = "/metadata";
    static constexpr auto PATH_CALENDAR_ENTRIES = "/calendar-entries";
    static constexpr auto PATH_FLASHCARDS = "/flashcards/due";

    static QUrl buildUrl(const QString& path);

    QNetworkAccessManager manager;
};

#endif //FRONTEND_APICLIENT_H
