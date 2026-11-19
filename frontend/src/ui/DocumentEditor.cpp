#include "DocumentEditor.h"

#include <QPainter>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineView>

SaveStatus::SaveStatus(QWidget *parent) : QWidget(parent) {
    setFixedSize(24, 24);
    connect(&animationTimer, &QTimer::timeout, this, [this] {
        angle = (angle + 30) % 360;
        update();
    });
}

void SaveStatus::setState(const State newState) {
    state = newState;
    if (state == Saving) {
        animationTimer.start(80);
        setToolTip("Wird gespeichert");
    } else {
        animationTimer.stop();
        setToolTip(
            state == Saved ? "Gespeichert"
                           : state == Error ? "Speichern fehlgeschlagen" : ""
        );
    }
    update();
}

void SaveStatus::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (state == Saving) {
        QPen pen(QColor(70, 145, 230), 2);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawArc(rect().adjusted(4, 4, -4, -4), (90 - angle) * 16, 270 * 16);
    } else if (state == Saved) {
        painter.setPen(QPen(QColor(90, 140, 195), 1.5));
        painter.drawLine(5, 12, 10, 17);
        painter.drawLine(10, 17, 19, 7);
    } else if (state == Error) {
        painter.setPen(QColor(220, 80, 60));
        painter.setFont(QFont(painter.font().family(), 16));
        painter.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("\xE2\x9A\xA0"));
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
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(webView);

    webChannel->registerObject(QStringLiteral("documentBridge"), bridge);
    webView->page()->setWebChannel(webChannel);

    connect(bridge, &DocumentEditorBridge::ready, this, [this] {
        editorReady = true;
        emit bridge->setMarkdownRequested(currentMarkdown, currentRevision);
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
    if (editorReady) {
        emit bridge->setMarkdownRequested(markdown, currentRevision);
    }
}

void DocumentEditor::clear() {
    setMarkdown({});
}
