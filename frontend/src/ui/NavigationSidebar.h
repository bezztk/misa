#ifndef FRONTEND_NAVIGATIONSIDEBAR_H
#define FRONTEND_NAVIGATIONSIDEBAR_H

#include <QFrame>

#include <functional>

class ApiClient;
class DomainView;
class QLabel;
class QContextMenuEvent;
class QIcon;
class QMouseEvent;
class QToolButton;
class QVBoxLayout;
class QWidget;

namespace NavigationIcons {
inline constexpr auto tasks = ":/assets/icons/task-white.png";
inline constexpr auto contexts = ":/assets/icons/category.png";
inline constexpr auto calendar = ":/assets/icons/calendar.png";
inline constexpr auto ideas = ":/assets/icons/idea.png";
inline constexpr auto learning = ":/assets/icons/flashcards-white.png";
inline constexpr auto rss = ":/assets/icons/rss.png";
inline constexpr auto sources = ":/assets/icons/text-analytics.png";
inline constexpr auto media = ":/assets/icons/video-camera.png";
inline constexpr auto notes = ":/assets/icons/quill-pen.png";

QIcon tabIcon(const QString &path);
}

class NavigationCard : public QFrame {
    Q_OBJECT

public:
    explicit NavigationCard(
        const QString &title,
        const QString &iconPath,
        bool collapsible = false,
        QWidget *parent = nullptr
    );
    explicit NavigationCard(
        const QString &title,
        const QIcon &icon,
        bool collapsible,
        QWidget *parent
    );

protected:
    QVBoxLayout *contentLayout() const;
    void setSubtext(const QString &text);
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

signals:
    void activated();
    void contextMenuRequested(const QPoint &globalPosition);

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void setExpanded(bool expanded);

    bool collapsible;
    bool expanded = true;
    QWidget *header;
    QWidget *content;
    QLabel *subtextLabel;
    QToolButton *expandButton = nullptr;
    QVBoxLayout *moduleContentLayout;
};

class TasksNavigationSection final : public NavigationCard {
    Q_OBJECT

public:
    explicit TasksNavigationSection(ApiClient &apiClient, QWidget *parent = nullptr);

signals:
    void openRequested();
    void createRequested();
};

class CalendarNavigationSection final : public NavigationCard {
    Q_OBJECT

public:
    explicit CalendarNavigationSection(QWidget *parent = nullptr);

signals:
    void openRequested();
    void createRequested();
    void refreshRequested();
};

class IdeasNavigationSection final : public NavigationCard {
    Q_OBJECT

public:
    explicit IdeasNavigationSection(ApiClient &apiClient, QWidget *parent = nullptr);

signals:
    void openRequested();
    void createRequested();
};

class LearningNavigationSection final : public NavigationCard {
    Q_OBJECT

public:
    explicit LearningNavigationSection(ApiClient &apiClient, QWidget *parent = nullptr);

signals:
    void openRequested();
    void createSetRequested();
    void refreshRequested();
};

class RssNavigationSection final : public NavigationCard {
    Q_OBJECT

public:
    explicit RssNavigationSection(ApiClient &apiClient, QWidget *parent = nullptr);

signals:
    void openRequested();
    void createFeedRequested();
    void createCategoryRequested();
    void refreshAllRequested();
};

class MediaNavigationSection final : public NavigationCard {
    Q_OBJECT

public:
    explicit MediaNavigationSection(QWidget *parent = nullptr);

signals:
    void openRequested();
    void importRequested();
};

class ActualizationNavigationSection final : public NavigationCard {
    Q_OBJECT

public:
    using BeforeNodeActionHandler = std::function<
        void(int, std::function<void()>)
    >;

    explicit ActualizationNavigationSection(
        ApiClient &apiClient,
        QWidget *parent = nullptr
    );
    void setBeforeNodeActionHandler(BeforeNodeActionHandler handler);

signals:
    void openDomainNodeRequested(int nodeId, const QString &title);

private:
    DomainView *domainNavigation;
};

class NavigationSidebar final : public QWidget {
    Q_OBJECT

public:
    using BeforeNodeActionHandler =
        ActualizationNavigationSection::BeforeNodeActionHandler;

    explicit NavigationSidebar(ApiClient &apiClient, QWidget *parent = nullptr);
    void setBeforeNodeActionHandler(BeforeNodeActionHandler handler);

signals:
    void openTasksRequested();
    void openContextsRequested();
    void createContextRequested();
    void openCalendarRequested();
    void openIdeasRequested();
    void openLearningRequested();
    void createLearningSetRequested();
    void openRssRequested();
    void openSourcesRequested();
    void createSourceRequested();
    void createSourceCategoryRequested();
    void openMediaRequested();
    void importMediaRequested();
    void createRssFeedRequested();
    void createRssCategoryRequested();
    void refreshAllRssRequested();
    void createTaskRequested();
    void createCalendarEntryRequested();
    void refreshCalendarRequested();
    void createIdeaRequested();
    void refreshLearningRequested();
    void openDomainNodeRequested(int nodeId, const QString &title);

private:
    ActualizationNavigationSection *actualizationSection;
};

#endif // FRONTEND_NAVIGATIONSIDEBAR_H
