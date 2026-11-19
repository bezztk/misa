#ifndef FRONTEND_DOCUMENTEDITOR_H
#define FRONTEND_DOCUMENTEDITOR_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QWidget>

class QPaintEvent;
class QResizeEvent;
class QWebChannel;
class QWebEngineView;
class EditorLoadingOverlay;

class SaveStatus : public QWidget {
public:
    enum State { Hidden, Saving, Saved, Error };

    explicit SaveStatus(QWidget *parent = nullptr);
    void setState(State newState);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    State state = Hidden;
    QTimer animationTimer;
    int angle = 0;
};

class DocumentEditorBridge final : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

public slots:
    void editorReady();
    void contentChanged(const QString &markdown, int revision);
    void documentApplied(int revision);

signals:
    void ready();
    void markdownEdited(const QString &markdown, int revision);
    void markdownApplied(int revision);
    void setMarkdownRequested(const QString &markdown, int revision);
};

class DocumentEditor final : public QWidget {
    Q_OBJECT

public:
    explicit DocumentEditor(QWidget *parent = nullptr);

    [[nodiscard]] QString markdown() const;
    void setMarkdown(const QString &markdown);

signals:
    void markdownChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void scheduleMarkdownUpdate();
    void startLoading();
    void finishLoadingIfReady();

    QWebEngineView *webView;
    QWebChannel *webChannel;
    DocumentEditorBridge *bridge;
    EditorLoadingOverlay *loadingOverlay;
    QTimer webViewResizeTimer;
    QTimer markdownUpdateTimer;
    QTimer minimumLoadingTimer;
    QString currentMarkdown;
    int currentRevision = 0;
    bool editorReady = false;
    bool markdownUpdatePending = false;
    bool editorLoading = false;
    bool currentRevisionApplied = false;
    bool minimumLoadingTimeElapsed = false;
};

#endif // FRONTEND_DOCUMENTEDITOR_H
