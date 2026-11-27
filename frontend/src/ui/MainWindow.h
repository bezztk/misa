#ifndef FRONTEND_MAINWINDOW_H
#define FRONTEND_MAINWINDOW_H

#include <QMainWindow>
#include "../api/ApiClient.h"

class QCloseEvent;
class QLabel;
class QResizeEvent;
class QTimer;
class WorkspaceManager;
class TaskView;
class CalendarView;
class IdeaView;
class LearningArea;
class MediaView;
class RssView;
class SourceView;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void showToast(const QString &message);
    void positionToast();
    TaskView *openTasks();
    void openContexts();
    CalendarView *openCalendar();
    IdeaView *openIdeas();
    LearningArea *openLearning();
    RssView *openRss();
    SourceView *openSources();
    MediaView *openMedia();
    void openDomainNode(int nodeId, const QString &title);

    ApiClient apiClient;
    WorkspaceManager *workspaceManager;
    QLabel *toastLabel;
    QTimer *toastTimer;
    bool startupComplete = false;
    bool closeAfterSave = false;
};

#endif //FRONTEND_MAINWINDOW_H
