#include "DocumentEditor.h"
#include "WorkspaceSplitter.h"

#include <QFontDatabase>
#include <QPainter>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTextDocument>
#include <QTextEdit>
#include <QVBoxLayout>

SaveStatus::SaveStatus(QWidget *parent) : QWidget(parent) {
    setFixedHeight(22);
    setMinimumWidth(188);
    QFont statusFont = font();
    statusFont.setPixelSize(11);
    setFont(statusFont);
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
    painter.setFont(font());

    if (state == Hidden) {
        return;
    }

    if (state == Saving) {
        const QColor color("#91ADC2");
        QPen pen(color, 2);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        const QString text = "Wird synchronisiert …";
        const int textWidth = painter.fontMetrics().horizontalAdvance(text);
        const int spinnerLeft = width() - textWidth - 22;
        painter.drawArc(
            QRect(spinnerLeft, 4, 14, 14),
            (90 - angle) * 16,
            270 * 16
        );
        painter.drawText(
            rect(),
            Qt::AlignRight | Qt::AlignVCenter,
            text
        );
    } else if (state == Saved) {
        painter.setPen(QColor("#66BB6A"));
        painter.drawText(rect(), Qt::AlignRight | Qt::AlignVCenter, "Synchronisiert");
    } else if (state == Error) {
        painter.setPen(QColor("#DC503C"));
        painter.drawText(
            rect(),
            Qt::AlignRight | Qt::AlignVCenter,
            "Synchronisierung fehlgeschlagen"
        );
    }
}

DocumentEditor::DocumentEditor(QWidget *parent)
    : QWidget(parent), sourceEdit(new QTextEdit), preview(new QTextEdit) {
    sourceEdit->setAcceptRichText(false);
    sourceEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    sourceEdit->setAccessibleName(tr("Markdown-Quelltext"));
    sourceEdit->setPlaceholderText(tr("Dokument bearbeiten …"));
    sourceEdit->setFrameShape(QFrame::NoFrame);
    sourceEdit->document()->setDocumentMargin(12);

    preview->setReadOnly(true);
    preview->setAccessibleName(tr("Markdown-Vorschau"));
    preview->setFrameShape(QFrame::NoFrame);
    preview->document()->setDocumentMargin(12);

    auto *splitter = new WorkspaceSplitter(Qt::Horizontal, this);
    splitter->addWidget(sourceEdit);
    splitter->addWidget(preview);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);
    setFocusProxy(sourceEdit);

    connect(sourceEdit, &QTextEdit::textChanged, this, [this] {
        auto *scrollBar = preview->verticalScrollBar();
        const int scrollPosition = scrollBar->value();
        preview->setMarkdown(markdown());
        scrollBar->setValue(scrollPosition);
        emit markdownChanged();
    });
}

QString DocumentEditor::markdown() const {
    QString text = sourceEdit->document()->toRawText();
    return text.replace(QChar::ParagraphSeparator, u'\n');
}

void DocumentEditor::setMarkdown(const QString &markdown) {
    const QSignalBlocker blocker(sourceEdit);
    sourceEdit->setPlainText(markdown);
    preview->setMarkdown(markdown);
}
