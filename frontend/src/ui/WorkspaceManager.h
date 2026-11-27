#ifndef FRONTEND_WORKSPACEMANAGER_H
#define FRONTEND_WORKSPACEMANAGER_H

#include <QHash>
#include <QIcon>
#include <QPointer>
#include <QWidget>

#include <functional>

class QVBoxLayout;
class TabGroup;
class WorkspaceSplitter;

class WorkspaceManager final : public QWidget {
    Q_OBJECT

public:
    using ViewFactory = std::function<QWidget *()>;
    using CloseHandler = std::function<void(std::function<void()>)>;

    explicit WorkspaceManager(QWidget *parent = nullptr);

    QWidget *openView(
        const QString &key,
        const QString &title,
        const QIcon &icon,
        ViewFactory factory,
        CloseHandler closeHandler = {}
    );
    void closeView(const QString &key);
    void setViewTitle(const QString &key, const QString &title);
    void prepareView(const QString &key, std::function<void()> continuation);
    void prepareToClose(std::function<void()> continuation);

signals:
    void activeViewChanged(const QString &key);

private:
    struct ViewRecord {
        QPointer<QWidget> view;
        QPointer<TabGroup> group;
        QString title;
        QIcon icon;
        CloseHandler closeHandler;
        bool closing = false;
    };

    TabGroup *createGroup();
    TabGroup *targetGroup();
    TabGroup *splitGroup(TabGroup *group, Qt::Orientation orientation);
    void moveView(QWidget *view, TabGroup *target);
    void requestClose(QWidget *view);
    void finishClose(const QString &key);
    void removeEmptyGroup(TabGroup *group);
    void updateActiveView();
    void showTabContextMenu(
        TabGroup *group,
        QWidget *view,
        const QPoint &globalPosition
    );
    QString keyForView(QWidget *view) const;

    QVBoxLayout *workspaceLayout;
    QPointer<QWidget> root;
    QPointer<TabGroup> focusedGroup;
    QList<QPointer<TabGroup>> groups;
    QHash<QString, ViewRecord> views;
};

#endif // FRONTEND_WORKSPACEMANAGER_H
