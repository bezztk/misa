#include "TabGroup.h"

#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

TabGroup::TabGroup(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tabs = new QTabWidget(this);
    tabs->setDocumentMode(true);
    tabs->setMovable(true);
    tabs->setTabsClosable(true);
    tabs->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(tabs);

    connect(tabs, &QTabWidget::currentChanged, this, [this](const int index) {
        if (index >= 0) {
            emit focusRequested(this);
        }
    });
    connect(tabs, &QTabWidget::tabCloseRequested, this, [this](const int index) {
        if (QWidget *view = tabs->widget(index)) {
            emit tabCloseRequested(this, view);
        }
    });
    connect(
        tabs->tabBar(),
        &QWidget::customContextMenuRequested,
        this,
        [this](const QPoint &position) {
            const int index = tabs->tabBar()->tabAt(position);
            if (index < 0) {
                return;
            }
            emit tabContextMenuRequested(
                this,
                tabs->widget(index),
                tabs->tabBar()->mapToGlobal(position)
            );
        }
    );
}

void TabGroup::addView(QWidget *view, const QString &title) {
    tabs->addTab(view, title);
    tabs->setCurrentWidget(view);
    emit focusRequested(this);
}

void TabGroup::removeView(QWidget *view) {
    const int index = tabs->indexOf(view);
    if (index >= 0) {
        tabs->removeTab(index);
    }
}

void TabGroup::setCurrentView(QWidget *view) {
    tabs->setCurrentWidget(view);
    view->setFocus(Qt::OtherFocusReason);
    emit focusRequested(this);
}

void TabGroup::setViewTitle(QWidget *view, const QString &title) {
    const int index = tabs->indexOf(view);
    if (index >= 0) {
        tabs->setTabText(index, title);
    }
}

bool TabGroup::isEmpty() const {
    return tabs->count() == 0;
}
