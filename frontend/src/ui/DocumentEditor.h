#ifndef FRONTEND_DOCUMENTEDITOR_H
#define FRONTEND_DOCUMENTEDITOR_H

#include <QPlainTextEdit>
#include <QTimer>
#include <QWidget>

class QPaintEvent;
class QResizeEvent;

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

class DocumentEditor : public QPlainTextEdit {
public:
    explicit DocumentEditor(QWidget *parent = nullptr);
    int lineNumberAreaWidth() const;
    void paintLineNumbers(QPaintEvent *event);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);

    QWidget *lineNumberArea;
};

#endif // FRONTEND_DOCUMENTEDITOR_H
