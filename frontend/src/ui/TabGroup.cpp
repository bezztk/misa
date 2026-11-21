#include "TabGroup.h"

#include <QAbstractButton>
#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QPalette>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

static constexpr auto WORKSPACE_TABS_STYLE = R"(
    QTabWidget#workspaceTabs {
        background: #222222;
        border: none;
    }
    QTabWidget#workspaceTabs::pane {
        background: #1C1C1C;
        border: none;
    }
    QTabBar#workspaceTabBar {
        background: #222222;
        border: none;
        qproperty-drawBase: 0;
    }
    QTabBar#workspaceTabBar::tab {
        height: 37px;
        margin-right: 3px;
        padding: 0 7px 1px 12px;
        color: #9A9A9A;
        background: #222222;
        border: none;
        border-radius: 0;
    }
    QTabBar#workspaceTabBar::tab:hover {
        color: #D0D0D0;
        background: #292929;
    }
    QTabBar#workspaceTabBar::tab:selected {
        color: #E5E5E5;
        background: #1C1C1C;
        border: none;
    }
)";

static constexpr auto TAB_CLOSE_BUTTON_STYLE = R"(
    QAbstractButton {
        background: transparent;
        border: none;
        border-radius: 0;
        padding: 5px;
    }
    QAbstractButton:hover {
        background: #383838;
    }
    QAbstractButton:pressed {
        background: #424242;
    }
)";

class WorkspaceTabBar final : public QTabBar {
public:
    using QTabBar::QTabBar;

protected:
    void paintEvent(QPaintEvent *event) override {
        QTabBar::paintEvent(event);

        QPainter painter(this);
        painter.setPen(QColor("#3A3A3A"));
        for (int index = 0; index + 1 < count(); ++index) {
            const QRect current = tabRect(index);
            const QRect next = tabRect(index + 1);
            const int x = (current.right() + next.left()) / 2;
            const int centerY = current.center().y();
            painter.drawLine(x, centerY - 8, x, centerY + 8);
        }
    }
};

class WorkspaceTabWidget final : public QTabWidget {
public:
    explicit WorkspaceTabWidget(QWidget *parent = nullptr) : QTabWidget(parent) {
        setTabBar(new WorkspaceTabBar(this));
    }
};

TabGroup::TabGroup(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tabs = new WorkspaceTabWidget(this);
    tabs->setObjectName("workspaceTabs");
    tabs->setDocumentMode(true);
    tabs->setMovable(true);
    tabs->setTabsClosable(true);
    tabs->tabBar()->setObjectName("workspaceTabBar");
    tabs->setStyleSheet(WORKSPACE_TABS_STYLE);
    tabs->tabBar()->setFixedHeight(37);
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
    QPalette workspacePalette = view->palette();
    workspacePalette.setColor(QPalette::Window, QColor("#1C1C1C"));
    workspacePalette.setColor(QPalette::Base, QColor("#1C1C1C"));
    view->setPalette(workspacePalette);
    view->setAutoFillBackground(true);

    const int index = tabs->addTab(view, title);
    QAbstractButton *closeButton = qobject_cast<QAbstractButton *>(
        tabs->tabBar()->tabButton(index, QTabBar::RightSide)
    );
    if (!closeButton) {
        closeButton = qobject_cast<QAbstractButton *>(
            tabs->tabBar()->tabButton(index, QTabBar::LeftSide)
        );
    }
    if (closeButton) {
        closeButton->setFixedSize(24, 24);
        closeButton->setIcon(QIcon(":/assets/icons/close.png"));
        closeButton->setIconSize(QSize(14, 14));
        closeButton->setStyleSheet(TAB_CLOSE_BUTTON_STYLE);
    }
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
