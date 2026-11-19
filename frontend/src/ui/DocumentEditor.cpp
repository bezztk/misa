#include "DocumentEditor.h"

#include <QPainter>
#include <QResizeEvent>
#include <QUrl>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineView>

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

DocumentEditor::DocumentEditor(QWidget *parent)
    : QWidget(parent),
      webView(new QWebEngineView(this)),
      webChannel(new QWebChannel(this)),
      bridge(new DocumentEditorBridge(this)) {
    webView->setGeometry(rect());
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

    webView->setUrl(QUrl(QStringLiteral("qrc:/editor/index.html")));
}

QString DocumentEditor::markdown() const {
    return currentMarkdown;
}

void DocumentEditor::setMarkdown(const QString &markdown) {
    currentMarkdown = markdown;
    ++currentRevision;
    scheduleMarkdownUpdate();
}

void DocumentEditor::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
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
