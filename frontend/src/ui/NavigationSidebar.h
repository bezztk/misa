#ifndef FRONTEND_NAVIGATIONSIDEBAR_H
#define FRONTEND_NAVIGATIONSIDEBAR_H

#include <QWidget>

#include <functional>

class ApiClient;
class DomainView;
class QToolButton;
class QVBoxLayout;

class NavigationModuleSection : public QWidget {
    Q_OBJECT

public:
    explicit NavigationModuleSection(
        const QString &title,
        QWidget *parent = nullptr
    );

protected:
    QVBoxLayout *contentLayout() const;

private:
    QToolButton *headerButton;
    QWidget *content;
    QVBoxLayout *moduleContentLayout;
};

class TasksNavigationSection final : public NavigationModuleSection {
    Q_OBJECT

public:
    explicit TasksNavigationSection(QWidget *parent = nullptr);

signals:
    void openRequested();
};

class CalendarNavigationSection final : public NavigationModuleSection {
    Q_OBJECT

public:
    explicit CalendarNavigationSection(QWidget *parent = nullptr);

signals:
    void openRequested();
};

class LearningNavigationSection final : public NavigationModuleSection {
    Q_OBJECT

public:
    explicit LearningNavigationSection(QWidget *parent = nullptr);

signals:
    void openRequested();
};

class ActualizationNavigationSection final : public NavigationModuleSection {
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
    void openCalendarRequested();
    void openLearningRequested();
    void openDomainNodeRequested(int nodeId, const QString &title);

private:
    ActualizationNavigationSection *actualizationSection;
};

#endif // FRONTEND_NAVIGATIONSIDEBAR_H
