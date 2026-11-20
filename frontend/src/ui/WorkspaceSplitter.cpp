#include "WorkspaceSplitter.h"

WorkspaceSplitter::WorkspaceSplitter(
    const Qt::Orientation orientation,
    QWidget *parent
) : QSplitter(orientation, parent) {
    setChildrenCollapsible(false);
    setHandleWidth(1);
    setStyleSheet(
        "QSplitter::handle { background: rgba(255, 255, 255, 0.25); }"
    );
}
