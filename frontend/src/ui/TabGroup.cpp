#include "TabGroup.h"

#include <QColor>
#include <QCursor>
#include <QEvent>
#include <QIcon>
#include <QPalette>
#include <QPointer>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int tabSpacing = 12;
constexpr int tabCloseInset = 4;
constexpr int minimumTabWidth = 120;
constexpr int maximumTabWidth = 240;

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
        margin-right: %1px;
        padding: 0 6px 0 10px;
        color: #ABABAB;
        background: #222222;
        border: none;
        border-bottom: 2px solid transparent;
        border-radius: 0;
    }
    QTabBar#workspaceTabBar::tab:hover {
        color: #D0D0D0;
        background: #292929;
    }
    QTabBar#workspaceTabBar::tab:selected {
        color: #E5E5E5;
        background: #2B2B2B;
        border-bottom-color: #686868;
    }
)";

static constexpr auto TAB_CLOSE_BUTTON_STYLE = R"(
    QToolButton {
        background: transparent;
        border: none;
        border-radius: 0;
        padding: 5px;
    }
    QToolButton:hover {
        background: #383838;
    }
    QToolButton:pressed {
        background: #424242;
    }
)";

QString tabLabel(QString title) {
    return title.replace("&", "&&");
}

QString tabTooltip(const QString &title) {
    return "<qt>" + title.toHtmlEscaped() + "</qt>";
}

class TabCloseSlot final : public QWidget {
public:
    explicit TabCloseSlot(QWidget *parent) : QWidget(parent), button(new QToolButton(this)) {
        button->setFixedSize(24, 24);
        button->setIcon(QIcon(":/assets/icons/close.png"));
        button->setIconSize(QSize(14, 14));
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::NoFocus);
        button->setAccessibleName("Tab schließen");
        button->setStyleSheet(TAB_CLOSE_BUTTON_STYLE);
        button->hide();

        // Qt positions the slot relative to the tab rectangle, including its trailing margin.
        setFixedSize(button->width() + tabSpacing + tabCloseInset, button->height());
    }

    QToolButton *closeButton() const {
        return button;
    }

private:
    QToolButton *button;
};

class WorkspaceTabBar final : public QTabBar {
public:
    explicit WorkspaceTabBar(QWidget *parent) : QTabBar(parent) {
        setAttribute(Qt::WA_Hover);
        setMouseTracking(true);
    }

    void addCloseSlot(const int index, TabCloseSlot *slot) {
        setTabButton(index, RightSide, slot);
        slot->closeButton()->installEventFilter(this);
        updateHoveredCloseButton();
    }

protected:
    QSize tabSizeHint(const int index) const override {
        QSize size = QTabBar::tabSizeHint(index);
        size.setWidth(std::clamp(size.width(), minimumTabWidth + tabSpacing,
            maximumTabWidth + tabSpacing));
        return size;
    }

    QSize minimumTabSizeHint(const int index) const override {
        return {minimumTabWidth + tabSpacing, tabSizeHint(index).height()};
    }

    bool event(QEvent *event) override {
        const bool handled = QTabBar::event(event);
        switch (event->type()) {
            case QEvent::Enter:
            case QEvent::Leave:
            case QEvent::HoverEnter:
            case QEvent::HoverMove:
            case QEvent::HoverLeave:
                updateHoveredCloseButton();
                break;
            default:
                break;
        }
        return handled;
    }

    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
            updateHoveredCloseButton();
        }
        return QTabBar::eventFilter(watched, event);
    }

    void tabLayoutChange() override {
        QTabBar::tabLayoutChange();
        updateHoveredCloseButton();
    }

private:
    void updateHoveredCloseButton() {
        const QPoint position = mapFromGlobal(QCursor::pos());
        const int index = underMouse() ? tabAt(position) : -1;
        QToolButton *next = nullptr;
        if (index >= 0 && tabRect(index).adjusted(0, 0, -tabSpacing, 0).contains(position)) {
            auto *slot = static_cast<TabCloseSlot *>(tabButton(index, RightSide));
            const auto *child = childAt(position);
            if (slot && (!child || child == slot || child == slot->closeButton())) {
                next = slot->closeButton();
            }
        }
        if (next == hoveredCloseButton.data()) {
            return;
        }
        const auto previous = hoveredCloseButton;
        hoveredCloseButton = next;
        if (previous) {
            previous->hide();
        }
        if (next) {
            next->show();
        }
    }

    QPointer<QToolButton> hoveredCloseButton;
};

class WorkspaceTabWidget final : public QTabWidget {
public:
    explicit WorkspaceTabWidget(QWidget *parent = nullptr) : QTabWidget(parent) {
        setTabBar(new WorkspaceTabBar(this));
    }
};

} // namespace

TabGroup::TabGroup(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tabs = new WorkspaceTabWidget(this);
    tabs->setObjectName("workspaceTabs");
    tabs->setDocumentMode(true);
    tabs->setMovable(true);
    tabs->setElideMode(Qt::ElideRight);
    tabs->setUsesScrollButtons(true);
    tabs->setIconSize(QSize(16, 16));
    auto *bar = tabs->tabBar();
    bar->setObjectName("workspaceTabBar");
    bar->setExpanding(false);
    tabs->setStyleSheet(QString(WORKSPACE_TABS_STYLE).arg(tabSpacing));
    bar->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(tabs);

    connect(tabs, &QTabWidget::currentChanged, this, [this](const int index) {
        if (index >= 0) {
            emit focusRequested(this);
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

void TabGroup::addView(QWidget *view, const QString &title, const QIcon &icon) {
    QPalette workspacePalette = view->palette();
    workspacePalette.setColor(QPalette::Window, QColor("#1C1C1C"));
    workspacePalette.setColor(QPalette::Base, QColor("#1C1C1C"));
    view->setPalette(workspacePalette);
    view->setAutoFillBackground(true);

    const int index = tabs->addTab(view, icon, tabLabel(title));
    tabs->setTabToolTip(index, tabTooltip(title));
    auto *bar = static_cast<WorkspaceTabBar *>(tabs->tabBar());
    auto *closeSlot = new TabCloseSlot(bar);
    bar->addCloseSlot(index, closeSlot);
    connect(closeSlot->closeButton(), &QToolButton::clicked, this, [this, view] {
        emit tabCloseRequested(this, view);
    });
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
        tabs->setTabText(index, tabLabel(title));
        tabs->setTabToolTip(index, tabTooltip(title));
    }
}

bool TabGroup::isEmpty() const {
    return tabs->count() == 0;
}
