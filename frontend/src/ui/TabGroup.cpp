#include "TabGroup.h"

#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

static constexpr auto WORKSPACE_TABS_STYLE = R"(
    QTabWidget#workspaceTabs {
        background: #1C1C1C;
        border: none;
    }
    QTabWidget#workspaceTabs::pane {
        border: none;
        border-top: 1px solid #383838;
    }
    QTabBar#workspaceTabBar {
        background: #1C1C1C;
        border: none;
        qproperty-drawBase: 0;
    }
    QTabBar#workspaceTabBar::tab {
        height: 30px;
        padding: 0 12px;
        color: #A0A0A0;
        background: #222222;
        border: none;
        border-top: 2px solid transparent;
        border-right: 1px solid #383838;
        border-radius: 0;
    }
    QTabBar#workspaceTabBar::tab:hover {
        color: #E5E5E5;
        background: #303030;
    }
    QTabBar#workspaceTabBar::tab:selected {
        color: #E5E5E5;
        background: #292929;
        border-top-color: #708FA6;
    }
    QTabBar#workspaceTabBar::close-button {
        image: url(:/assets/icons/close-tab.svg);
        width: 12px;
        height: 12px;
        margin-left: 4px;
        margin-right: 2px;
        border: none;
    }
    QTabBar#workspaceTabBar::close-button:hover {
        background: #383838;
    }
)";

TabGroup::TabGroup(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tabs = new QTabWidget(this);
    tabs->setObjectName("workspaceTabs");
    tabs->setDocumentMode(true);
    tabs->setMovable(true);
    tabs->setTabsClosable(true);
    tabs->tabBar()->setObjectName("workspaceTabBar");
    tabs->setStyleSheet(WORKSPACE_TABS_STYLE);
    tabs->tabBar()->setFixedHeight(32);
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
