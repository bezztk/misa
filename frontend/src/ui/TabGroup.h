#ifndef FRONTEND_TABGROUP_H
#define FRONTEND_TABGROUP_H

#include <QWidget>

class QIcon;
class QTabWidget;

class TabGroup final : public QWidget {
    Q_OBJECT

public:
    explicit TabGroup(QWidget *parent = nullptr);

    void addView(QWidget *view, const QString &title, const QIcon &icon);
    void removeView(QWidget *view);
    void setCurrentView(QWidget *view);
    void setViewTitle(QWidget *view, const QString &title);
    [[nodiscard]] QWidget *currentView() const;
    [[nodiscard]] bool isEmpty() const;

signals:
    void focusRequested(TabGroup *group);
    void tabCloseRequested(TabGroup *group, QWidget *view);
    void tabContextMenuRequested(
        TabGroup *group,
        QWidget *view,
        const QPoint &globalPosition
    );

private:
    QTabWidget *tabs;
};

#endif // FRONTEND_TABGROUP_H
