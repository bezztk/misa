#ifndef FRONTEND_WORKSPACESPLITTER_H
#define FRONTEND_WORKSPACESPLITTER_H

#include <QSplitter>

class WorkspaceSplitter final : public QSplitter {
    Q_OBJECT

public:
    explicit WorkspaceSplitter(
        Qt::Orientation orientation,
        QWidget *parent = nullptr
    );
};

#endif // FRONTEND_WORKSPACESPLITTER_H
