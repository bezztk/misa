#ifndef FRONTEND_TASKVIEW_H
#define FRONTEND_TASKVIEW_H

#include "../api/ApiClient.h"
#include "../models/Task.h"

#include <QObject>
#include <QString>
#include <QWidget>

#include <vector>

class QResizeEvent;
class QWebChannel;
class QWebEngineView;

class TaskBridge final : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

public slots:
    void kanbanReady();
    void taskStateChangeRequested(int id, int stateId);
    void editTaskRequested(int id);
    void contextMenuRequested(int id, int x, int y);

signals:
    void ready();
    void changeTaskState(int id, int stateId);
    void editTask(int id);
    void showContextMenu(int id, int x, int y);
    void boardChanged(const QString &boardJson);
};

class TaskView : public QWidget {
public:
    explicit TaskView(ApiClient &apiClient, QWidget *parent = nullptr);
    void refreshTasks();
    void refreshMetadata();
    void openCreateDialog();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void pushBoardData();
    void openEditDialog(const Task &task);
    void openScheduleDialog(const Task &task);
    void showTaskContextMenu(int id, int x, int y);
    Task *taskById(int id);

    std::vector<Task> tasks;
    std::vector<DomainNode> domainNodes;
    ApiClient &apiClient;
    QWebEngineView *webView;
    QWebChannel *webChannel;
    TaskBridge *bridge;
    bool kanbanReady = false;
};

#endif // FRONTEND_TASKVIEW_H
