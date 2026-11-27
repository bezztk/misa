#ifndef FRONTEND_APICLIENT_H
#define FRONTEND_APICLIENT_H
#include "../models/Task.h"
#include "../models/Context.h"
#include "../models/Flashcard.h"
#include "../models/Attachment.h"
#include "../models/Metadata.h"
#include "../models/DomainNode.h"
#include "../models/Document.h"
#include "../models/Pursuit.h"
#include "../models/Progression.h"
#include "../models/CalendarEntry.h"
#include "../models/EntityFieldVersion.h"
#include "../models/Comment.h"
#include "../models/Idea.h"
#include "../models/Media.h"
#include "../models/Rss.h"
#include "../models/Source.h"
#include "../models/ShortTermLearning.h"
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

#include <functional>

class QJsonArray;
class QJsonDocument;
class QJsonObject;
class QNetworkReply;

class ApiClient : public QObject {
    Q_OBJECT

signals:
    void metadataReceived();
    void requestFailed(const QString& message);
    void taskReceived(const std::vector<Task>& tasks);
    void tasksLoadFailed();
    void taskCreated();
    void taskUpdated();
    void taskDeleted();
    void contextsReceived(const std::vector<Context> &contexts);
    void contextsChanged();
    void calendarEntriesReceived(const std::vector<CalendarEntry>& entries);
    void calendarEntryCreated();
    void calendarEntryUpdated();
    void calendarEntryDeleted();
    void routineExecutionsGenerated();
    void ideasReceived(const std::vector<Idea>& ideas);
    void ideaCreated();
    void ideaUpdated();
    void ideaDeleted();
    void mediaReceived(int requestId, const std::vector<Media> &media);
    void mediaLoadFailed(int requestId);
    void mediaImported(int requestId);
    void mediaImportFailed(int requestId, const QString &message);
    void mediaSeasonsReceived(int requestId, const std::vector<MediaSeason> &seasons);
    void mediaEpisodesReceived(int requestId, const std::vector<MediaEpisode> &episodes);
    void mediaSeriesProgressReceived(int requestId, const MediaSeriesProgress &progress);
    void mediaDetailsLoadFailed(int requestId);
    void mediaPersonalDataUpdateFinished(int requestId, bool successful);
    void mediaEpisodeWatchedUpdateFinished(int requestId, int entityId, int seasonId, bool successful);
    void mediaRatingUpdateFinished(int entityId, std::optional<MediaRating> rating, bool successful);
    void flashcardsReceived(const std::vector<FlashcardReviewQueueItem>& flashcards);
    void dueFlashcardCountReceived(int count);
    void dueFlashcardsLoadFailed();
    void flashcardRated();
    void flashcardRatingFailed();
    void nodeFlashcardsReceived(int nodeId, const std::vector<Flashcard>& flashcards);
    void nodeFlashcardsChanged(int nodeId);
    void flashcardCreated();
    void flashcardUpdated();
    void nodeAttachmentsReceived(int nodeId, const std::vector<Attachment>& attachments);
    void nodeAttachmentsChanged(int nodeId);
    void nodePursuitsReceived(int nodeId, const std::vector<Pursuit>& pursuits);
    void nodePursuitsChanged(int nodeId);
    void pursuitCreated();
    void pursuitUpdated();
    void progressionsReceived(const std::vector<Progression>& progressions,
                              const std::vector<ProgressionPursuit>& pursuits);
    void availableProgressionPursuitsReceived(
        int stageId,
        const std::vector<ProgressionPursuit>& pursuits
    );
    void progressionsChanged();
    void progressionCreated();
    void progressionUpdated();
    void progressionStageCreated();
    void pursuitAssigned();
    void domainTreeReceived(
        const std::vector<MetadataItem>& domains,
        const std::vector<DomainNode>& nodes
    );
    void domainUpdated(const MetadataItem& domain);
    void domainNodeCreated(const DomainNode& node);
    void domainNodeFieldUpdated(int nodeId, DomainNodeField field, const QJsonValue& value);
    void domainNodeDeleted(int nodeId);
    void documentCreated(const Document& document);
    void documentReceived(const Document& document);
    void documentLoadFailed(int nodeId);
    void documentUpdated(int nodeId, bool successful);
    void documentDeleted(int nodeId);
    void documentVersionsReceived(
        int nodeId,
        const std::vector<DocumentVersion>& versions
    );
    void documentVersionsLoadFailed(int nodeId);
    void documentVersionRestored(const Document& document);
    void documentVersionRestoreFailed(int nodeId);
    void entityFieldVersionsReceived(
        int entityId,
        const std::vector<EntityFieldVersion>& versions
    );
    void entityFieldVersionsLoadFailed(int entityId);
    void commentCreated(int entityId, const Comment& comment);
    void commentUpdated(const Comment& comment);
    void commentDeleted(int id);
    void rssFeedsReceived(const std::vector<RssFeed>& feeds);
    void rssEntriesReceived(const std::vector<RssEntry>& entries);
    void rssFeedCreated();
    void rssFeedUpdated();
    void rssFeedDeleted();
    void rssFeedsRefreshed();
    void rssFeedEntriesRead();
    void rssEntryUpdated();
    void sourcesReceived(const std::vector<Source> &sources);
    void sourcesChanged();
    void sourceCategoriesReceived(const std::vector<SourceCategory> &categories);
    void sourceCategoriesChanged();

public:
    explicit ApiClient(QObject *parent = nullptr);
    const Metadata& metadata() const;
    void getMetadata();
    void getTasks();
    void createTask(const CreateTask& task);
    void updateTaskField(int id, TaskField field, const QJsonValue& value);
    void updateTaskContexts(int id, const std::vector<int> &contextIds);
    void deleteTask(int id);
    void getContexts();
    void createContext(const QString &title, const QByteArray &icon, const QColor &color);
    void updateContext(int id, const QString &title, const QByteArray &icon, const QColor &color);
    void deleteContext(int id);
    void getCalendarEntries();
    void createCalendarEntry(const CreateCalendarEntry& entry);
    void updateCalendarEntry(int id, const CreateCalendarEntry& entry);
    void deleteCalendarEntry(int id);
    void generateRoutineExecutions();
    void getIdeas();
    void createIdea(const CreateIdea& idea);
    void updateIdeaField(int id, IdeaField field, const QJsonValue& value);
    void deleteIdea(int id);
    int getMedia();
    int importMedia(const QString &url);
    int getMediaSeasons(int entityId);
    int getMediaEpisodes(int entityId, int seasonId);
    int getMediaSeriesProgress(int entityId);
    int updateMediaPersonalData(int entityId, const QJsonObject &fields);
    int updateMediaEpisodeWatched(int entityId, int seasonId, int episodeId, bool isWatched);
    void updateMediaWatchStatus(int entityId, int watchStatusId);
    void updateMediaRating(int entityId, std::optional<MediaRating> rating);
    void getRssFeeds();
    void createRssFeed(const QString& url);
    void updateRssFeed(int id, const QString& url, bool enabled);
    void refreshRssFeed(int id);
    void refreshAllRssFeeds();
    void markRssFeedRead(int id);
    void deleteRssFeed(int id);
    void getRssEntries();
    void updateRssEntryRead(int id, bool isRead);
    void getSources();
    void createSource(const QString &url);
    void updateSourceTitle(const QString &id, const QString &title);
    void assignSourceCategory(const QString &id, int categoryId);
    void deleteSource(const QString &id);
    void getSourceCategories();
    void createSourceCategory(const QString &name);
    void renameSourceCategory(int id, const QString &name);
    void deleteSourceCategory(int id);
    void getDueFlashcards();
    void getDueFlashcardCount();
    void rateFlashcard(int id, ReviewFlashcard reviewFlashcard);
    void getNodeFlashcards(int nodeId);
    void createFlashcard(int nodeId, const QString& front, const QString& back);
    void updateFlashcardField(int nodeId, int id, FlashcardField field,
                              const QJsonValue& value);
    void deleteFlashcard(int nodeId, int id);
    void getShortTermLearningSets(QObject &context,
        std::function<void(std::optional<std::vector<ShortTermLearningSet>>)> received);
    void createShortTermLearningSet(const QString &title, bool bidirectional,
        QObject &context, std::function<void(bool)> finished);
    void deleteShortTermLearningSet(int id, QObject &context, std::function<void(bool)> finished);
    void resetShortTermLearningSet(int id, QObject &context, std::function<void(bool)> finished);
    void getShortTermLearningCards(int setId, QObject &context,
        std::function<void(std::optional<ShortTermLearningCards>)> received);
    void importShortTermLearningCards(int setId, const QJsonArray &cards,
        QObject &context, std::function<void(bool)> finished);
    void updateShortTermLearningCard(const ShortTermLearningCard &card,
        QObject &context, std::function<void(bool)> finished);
    void deleteShortTermLearningCard(int id, QObject &context, std::function<void(bool)> finished);
    void rateShortTermLearningCard(int id, bool reverse, ShortTermLearningRating rating,
        QObject &context, std::function<void(std::optional<ShortTermLearningProgress>)> received);
    void getNodeAttachments(int nodeId);
    void createAttachment(int nodeId, const QString& originalName,
                          const QString& storageName, qint64 sizeBytes);
    void deleteAttachment(int nodeId, int id);
    void getNodePursuits(int nodeId);
    void createPursuit(int nodeId, const QString& name);
    void updatePursuitRequirements(int nodeId, int pursuitId,
                                   const UpdatePursuitRequirements& requirements);
    void getProgressions();
    void getAvailableProgressionPursuits(int stageId);
    void createProgression(const QString& name);
    void updateProgressionField(int id, ProgressionField field, const QJsonValue& value);
    void createProgressionStage(int progressionId, const QString& name);
    void assignPursuitToStage(int stageId, int pursuitId);
    void getDomainTree();
    void updateDomain(int id, const std::optional<QString>& color,
                      const std::optional<QByteArray>& icon);
    void createDomainNode(int parentId, const QString& name);
    void createRootDomainNode(int domainId, const QString& name);
    void deleteDomainNode(int id);
    void updateDomainNodeField(int id, DomainNodeField field, const QJsonValue& value);
    void createDocument(int nodeId);
    void getDocument(int nodeId);
    void updateDocument(int nodeId, const QString& content);
    void deleteDocument(int nodeId);
    void getDocumentVersions(int nodeId);
    void restoreDocumentVersion(int nodeId, int version);
    void getEntityFieldVersions(int entityId);
    QNetworkReply *createComment(int entityId, const QString& content);
    QNetworkReply *updateComment(int id, const QString& content);
    void deleteComment(int id);

private:
    static constexpr auto HANDLE_ERROR_LOCALLY = QNetworkRequest::User;
    static constexpr auto SCHEME = "http";
    static constexpr auto HOST = "localhost";
    static constexpr int PORT = 3000;
    static constexpr auto BASE_PATH = "/api";
    static constexpr auto PATH_TASKS = "/tasks";
    static constexpr auto PATH_CONTEXTS = "/contexts";
    static constexpr auto PATH_METADATA = "/metadata";
    static constexpr auto PATH_CALENDAR_ENTRIES = "/calendar-entries";
    static constexpr auto PATH_IDEAS = "/ideas";
    static constexpr auto PATH_MEDIA = "/media";
    static constexpr auto PATH_FLASHCARDS = "/flashcards/due";
    static constexpr auto PATH_RSS_FEEDS = "/rss/feeds";
    static constexpr auto PATH_RSS_ENTRIES = "/rss/entries";
    static constexpr auto PATH_SOURCES = "/sources";
    static constexpr auto PATH_SOURCE_CATEGORIES = "/source-categories";

    static QUrl buildUrl(const QString& path);
    void requestDueFlashcards(bool includeFlashcards);
    bool requestSucceeded(const QNetworkReply *reply) const;
    bool readJson(QNetworkReply *reply, const QString& operation, QJsonDocument& document);
    void sendChange(const QString &path, const QByteArray &method,
                    const QJsonObject &body, void (ApiClient::*changed)());
    void sendShortTermLearningChange(const QString &path, const QByteArray &method,
        const QJsonDocument &body, QObject &context, std::function<void(bool)> finished);
    int getMediaJson(const QString &path, const QString &operation,
                     std::function<bool(int, const QJsonDocument &)> received);
    int patchMediaJson(const QString &path, const QJsonObject &fields,
                       std::function<void(int, bool)> finished);

    QNetworkAccessManager manager;
    Metadata loadedMetadata;
    bool metadataLoaded = false;
    bool metadataLoading = false;
    int mediaRequestSequence = 0;
};

#endif //FRONTEND_APICLIENT_H
