#ifndef FRONTEND_NAVIGATIONSIDEBAR_H
#define FRONTEND_NAVIGATIONSIDEBAR_H

#include <QFrame>

#include <functional>

class ApiClient;
class DomainView;
class QLabel;
class QMouseEvent;
class QToolButton;
class QVBoxLayout;
class QWidget;

class NavigationCard : public QFrame {
    Q_OBJECT

public:
    explicit NavigationCard(
        const QString &title,
        const QString &iconPath,
        bool collapsible = false,
        QWidget *parent = nullptr
    );

protected:
    QVBoxLayout *contentLayout() const;
    void setSubtext(const QString &text);
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void activated();

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
};

class CalendarNavigationSection final : public NavigationCard {
    Q_OBJECT

public:
    explicit CalendarNavigationSection(QWidget *parent = nullptr);

signals:
    void openRequested();
};

class LearningNavigationSection final : public NavigationCard {
    Q_OBJECT

public:
    explicit LearningNavigationSection(ApiClient &apiClient, QWidget *parent = nullptr);

signals:
    void openRequested();
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
    void openCalendarRequested();
    void openLearningRequested();
    void openDomainNodeRequested(int nodeId, const QString &title);

private:
    ActualizationNavigationSection *actualizationSection;
};

#endif // FRONTEND_NAVIGATIONSIDEBAR_H
