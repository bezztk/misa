#ifndef FRONTEND_DOCUMENTEDITOR_H
#define FRONTEND_DOCUMENTEDITOR_H

#include <QString>
#include <QTimer>
#include <QWidget>

class QPaintEvent;
class QTextEdit;

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

class DocumentEditor final : public QWidget {
    Q_OBJECT

public:
    explicit DocumentEditor(QWidget *parent = nullptr);

    [[nodiscard]] QString markdown() const;
    void setMarkdown(const QString &markdown);

signals:
    void markdownChanged();

private:
    QTextEdit *sourceEdit;
    QTextEdit *preview;
};

#endif // FRONTEND_DOCUMENTEDITOR_H
