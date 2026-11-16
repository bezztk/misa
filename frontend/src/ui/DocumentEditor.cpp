#include "DocumentEditor.h"

#include <QPainter>
#include <QResizeEvent>
#include <QTextBlock>

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(DocumentEditor *editor)
        : QWidget(editor), editor(editor) {}

    QSize sizeHint() const override {
        return {editor->lineNumberAreaWidth(), 0};
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        editor->paintLineNumbers(event);
    }

private:
    DocumentEditor *editor;
};

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

DocumentEditor::DocumentEditor(QWidget *parent)
    : QPlainTextEdit(parent), lineNumberArea(new LineNumberArea(this)) {
    connect(this, &QPlainTextEdit::blockCountChanged, this,
            [this] { updateLineNumberAreaWidth(); });
    connect(this, &QPlainTextEdit::updateRequest, this,
            [this](const QRect &rect, const int dy) {
                updateLineNumberArea(rect, dy);
            });
    updateLineNumberAreaWidth();
}

int DocumentEditor::lineNumberAreaWidth() const {
    int digits = 1;
    for (int count = qMax(1, blockCount()); count >= 10; count /= 10) {
        ++digits;
    }
    return 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void DocumentEditor::updateLineNumberAreaWidth() {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void DocumentEditor::updateLineNumberArea(const QRect &rect, const int dy) {
    if (dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }
    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth();
    }
}

void DocumentEditor::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);
    const QRect contents = contentsRect();
    lineNumberArea->setGeometry(
        QRect(contents.left(), contents.top(), lineNumberAreaWidth(), contents.height())
    );
}

void DocumentEditor::paintLineNumbers(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), palette().color(QPalette::Base));
    painter.setPen(palette().color(QPalette::Light));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.drawText(
                0,
                top,
                lineNumberArea->width() - 4,
                fontMetrics().height(),
                Qt::AlignRight,
                QString::number(blockNumber + 1)
            );
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
