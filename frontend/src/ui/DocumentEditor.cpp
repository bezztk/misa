#include "DocumentEditor.h"

#include <QPainter>
#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QUrl>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineView>

class EditorLoadingOverlay final : public QWidget {
public:
    explicit EditorLoadingOverlay(QWidget *parent = nullptr) : QWidget(parent) {
        connect(&animationTimer, &QTimer::timeout, this, [this] {
            angle = (angle + 30) % 360;
            update();
        });
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.fillRect(rect(), palette().color(QPalette::Base));
        painter.setRenderHint(QPainter::Antialiasing);

        const QPoint center(width() / 2, height() / 2 - 12);
        QPen pen(QColor(70, 145, 230), 3);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawArc(
            QRect(center.x() - 10, center.y() - 10, 20, 20),
            (90 - angle) * 16,
            270 * 16
        );

        painter.setPen(palette().color(QPalette::Text));
        painter.drawText(
            QRect(0, center.y() + 20, width(), 24),
            Qt::AlignHCenter | Qt::AlignTop,
            "Dokument wird geladen …"
        );
    }

    void showEvent(QShowEvent *event) override {
        QWidget::showEvent(event);
        animationTimer.start(80);
    }

    void hideEvent(QHideEvent *event) override {
        animationTimer.stop();
        QWidget::hideEvent(event);
    }

private:
    QTimer animationTimer;
    int angle = 0;
};

SaveStatus::SaveStatus(QWidget *parent) : QWidget(parent) {
    setFixedHeight(24);
    setMinimumWidth(210);
    connect(&animationTimer, &QTimer::timeout, this, [this] {
        angle = (angle + 30) % 360;
        update();
    });
}

void SaveStatus::setState(const State newState) {
    state = newState;
    if (state == Saving) {
        animationTimer.start(80);
        setToolTip("Wird synchronisiert");
    } else {
        animationTimer.stop();
        setToolTip(
            state == Saved ? "Synchronisiert"
                           : state == Error ? "Synchronisierung fehlgeschlagen" : ""
        );
    }
    update();
}

void SaveStatus::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (state == Hidden) {
        return;
    }

    if (state == Saving) {
        const QColor color(70, 145, 230);
        QPen pen(color, 2);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        const QString text = "Wird synchronisiert …";
        const int textWidth = painter.fontMetrics().horizontalAdvance(text);
        const int spinnerLeft = width() - textWidth - 24;
        painter.drawArc(
            QRect(spinnerLeft, 4, 16, 16),
            (90 - angle) * 16,
            270 * 16
        );
        painter.drawText(
            rect(),
            Qt::AlignRight | Qt::AlignVCenter,
            text
        );
    } else if (state == Saved) {
        painter.setPen(QColor(102, 187, 106));
        painter.drawText(rect(), Qt::AlignRight | Qt::AlignVCenter, "Synchronisiert");
    } else if (state == Error) {
        painter.setPen(QColor(220, 80, 60));
        painter.drawText(
            rect(),
            Qt::AlignRight | Qt::AlignVCenter,
            "Synchronisierung fehlgeschlagen"
        );
    }
}

void DocumentEditorBridge::editorReady() {
    emit ready();
}

void DocumentEditorBridge::contentChanged(const QString &markdown, const int revision) {
    emit markdownEdited(markdown, revision);
}

void DocumentEditorBridge::documentApplied(const int revision) {
    emit markdownApplied(revision);
}

DocumentEditor::DocumentEditor(QWidget *parent)
    : QWidget(parent),
      webView(new QWebEngineView(this)),
      webChannel(new QWebChannel(this)),
      bridge(new DocumentEditorBridge(this)),
      loadingOverlay(new EditorLoadingOverlay(this)) {
    webView->setGeometry(rect());
    loadingOverlay->setGeometry(rect());
    loadingOverlay->hide();
    webViewResizeTimer.setSingleShot(true);
    webViewResizeTimer.setInterval(250);
    connect(&webViewResizeTimer, &QTimer::timeout, this, [this] {
        webView->setGeometry(rect());
    });
    markdownUpdateTimer.setSingleShot(true);
    markdownUpdateTimer.setInterval(300);
    connect(&markdownUpdateTimer, &QTimer::timeout, this, [this] {
        if (!markdownUpdatePending || !editorReady) {
            return;
        }
        markdownUpdatePending = false;
        emit bridge->setMarkdownRequested(currentMarkdown, currentRevision);
    });
    minimumLoadingTimer.setSingleShot(true);
    minimumLoadingTimer.setInterval(300);
    connect(&minimumLoadingTimer, &QTimer::timeout, this, [this] {
        minimumLoadingTimeElapsed = true;
        finishLoadingIfReady();
    });

    webChannel->registerObject(QStringLiteral("documentBridge"), bridge);
    webView->page()->setWebChannel(webChannel);

    connect(bridge, &DocumentEditorBridge::ready, this, [this] {
        editorReady = true;
        scheduleMarkdownUpdate();
    });
    connect(
        bridge,
        &DocumentEditorBridge::markdownEdited,
        this,
        [this](const QString &markdown, const int revision) {
            if (revision != currentRevision || markdown == currentMarkdown) {
                return;
            }
            currentMarkdown = markdown;
            emit markdownChanged();
        }
    );
    connect(bridge, &DocumentEditorBridge::markdownApplied, this,
            [this](const int revision) {
                if (revision != currentRevision) {
                    return;
                }
                currentRevisionApplied = true;
                finishLoadingIfReady();
            });

    webView->setUrl(QUrl(QStringLiteral("qrc:/editor/index.html")));
}

QString DocumentEditor::markdown() const {
    return currentMarkdown;
}

void DocumentEditor::setMarkdown(const QString &markdown) {
    currentMarkdown = markdown;
    ++currentRevision;
    startLoading();
    scheduleMarkdownUpdate();
}

void DocumentEditor::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    loadingOverlay->setGeometry(rect());
    webViewResizeTimer.start();
    if (markdownUpdatePending) {
        markdownUpdateTimer.start();
    }
}

void DocumentEditor::scheduleMarkdownUpdate() {
    markdownUpdatePending = true;
    if (editorReady) {
        markdownUpdateTimer.start();
    }
}

void DocumentEditor::startLoading() {
    editorLoading = true;
    currentRevisionApplied = false;
    minimumLoadingTimeElapsed = false;
    minimumLoadingTimer.start();
    webView->setEnabled(false);
    loadingOverlay->show();
    loadingOverlay->raise();
}

void DocumentEditor::finishLoadingIfReady() {
    if (!editorLoading || !currentRevisionApplied || !minimumLoadingTimeElapsed) {
        return;
    }

    editorLoading = false;
    loadingOverlay->hide();
    webView->setEnabled(true);
}
