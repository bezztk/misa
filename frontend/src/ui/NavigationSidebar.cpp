#include "NavigationSidebar.h"

#include "DomainView.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include <utility>

NavigationCard::NavigationCard(
    const QString &title,
    const QString &iconPath,
    const bool collapsible,
    QWidget *parent
) : QFrame(parent), collapsible(collapsible) {
    setObjectName("navigationCard");
    setAttribute(Qt::WA_Hover);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setMinimumHeight(collapsible ? 0 : 64);
    setStyleSheet(R"(
        QFrame#navigationCard {
            background: rgba(255, 255, 255, 0.035);
            border: 1px solid rgba(255, 255, 255, 0.13);
            border-radius: 7px;
        }
        QFrame#navigationCard:hover {
            background: rgba(255, 255, 255, 0.055);
            border-color: rgba(255, 255, 255, 0.21);
        }
        QWidget#navigationCardHeader, QLabel#navigationCardIcon,
        QLabel#navigationCardTitle, QToolButton#navigationCardExpand {
            background: transparent;
            border: none;
        }
        QLabel#navigationCardTitle {
            font-weight: bold;
        }
        QToolButton#navigationCardExpand {
            padding: 0;
        }
        QToolButton#navigationCardExpand:hover {
            background: rgba(255, 255, 255, 0.08);
            border-radius: 3px;
        }
    )");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    header = new QWidget(this);
    header->setObjectName("navigationCardHeader");
    header->setCursor(Qt::PointingHandCursor);
    header->installEventFilter(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(11, 9, 9, 9);
    headerLayout->setSpacing(8);

    auto *icon = new QLabel(header);
    icon->setObjectName("navigationCardIcon");
    icon->setFixedSize(18, 18);
    icon->setPixmap(QIcon(iconPath).pixmap(18, 18));
    icon->setAttribute(Qt::WA_TransparentForMouseEvents);
    headerLayout->addWidget(icon);

    auto *titleLabel = new QLabel(title, header);
    titleLabel->setObjectName("navigationCardTitle");
    titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    headerLayout->addWidget(titleLabel, 1);

    if (collapsible) {
        expandButton = new QToolButton(header);
        expandButton->setObjectName("navigationCardExpand");
        expandButton->setArrowType(Qt::DownArrow);
        expandButton->setAutoRaise(true);
        expandButton->setCursor(Qt::PointingHandCursor);
        expandButton->setToolTip("Einklappen");
        headerLayout->addWidget(expandButton);
        connect(expandButton, &QToolButton::clicked, this,
                [this] { setExpanded(!expanded); });
    }
    layout->addWidget(header);

    content = new QWidget(this);
    content->installEventFilter(this);
    moduleContentLayout = new QVBoxLayout(content);
    moduleContentLayout->setContentsMargins(11, 0, 11, 10);
    moduleContentLayout->setSpacing(6);
    layout->addWidget(content);

    if (!collapsible) {
        setCursor(Qt::PointingHandCursor);
    }
}

QVBoxLayout *NavigationCard::contentLayout() const {
    return moduleContentLayout;
}

void NavigationCard::mousePressEvent(QMouseEvent *event) {
    if (!collapsible && event->button() == Qt::LeftButton) {
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void NavigationCard::mouseReleaseEvent(QMouseEvent *event) {
    if (!collapsible && event->button() == Qt::LeftButton) {
        emit activated();
        event->accept();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

bool NavigationCard::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() != QEvent::MouseButtonPress
        && event->type() != QEvent::MouseButtonRelease) {
        return QFrame::eventFilter(watched, event);
    }

    const auto *mouseEvent = static_cast<QMouseEvent *>(event);
    if (mouseEvent->button() != Qt::LeftButton) {
        return QFrame::eventFilter(watched, event);
    }

    if (watched == header) {
        if (event->type() == QEvent::MouseButtonRelease) {
            if (collapsible) {
                setExpanded(!expanded);
            } else {
                emit activated();
            }
        }
        return true;
    }

    if (watched == content && !collapsible) {
        if (event->type() == QEvent::MouseButtonRelease) {
            emit activated();
        }
        return true;
    }

    return QFrame::eventFilter(watched, event);
}

void NavigationCard::setExpanded(const bool open) {
    expanded = open;
    content->setVisible(open);
    expandButton->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
    expandButton->setToolTip(open ? "Einklappen" : "Ausklappen");
}

TasksNavigationSection::TasksNavigationSection(QWidget *parent)
    : NavigationCard("Aufgaben", ":/assets/icons/task-white.png", false, parent) {
    connect(this, &NavigationCard::activated,
            this, &TasksNavigationSection::openRequested);
}

CalendarNavigationSection::CalendarNavigationSection(QWidget *parent)
    : NavigationCard("Kalender", ":/assets/icons/calendar.png", false, parent) {
    connect(this, &NavigationCard::activated,
            this, &CalendarNavigationSection::openRequested);
}

LearningNavigationSection::LearningNavigationSection(QWidget *parent)
    : NavigationCard(
        "Lernkarten", ":/assets/icons/flashcards-white.png", false, parent
    ) {
    connect(this, &NavigationCard::activated,
            this, &LearningNavigationSection::openRequested);
}

ActualizationNavigationSection::ActualizationNavigationSection(
    ApiClient &apiClient,
    QWidget *parent
) : NavigationCard(
        "Verwirklichung", ":/assets/icons/folder.png", true, parent
    ) {
    domainNavigation = new DomainView(apiClient, this);
    domainNavigation->setNodeOpenHandler(
        [this](const int nodeId, const QString &title) {
            emit openDomainNodeRequested(nodeId, title);
        }
    );
    contentLayout()->setContentsMargins(5, 0, 5, 6);
    contentLayout()->addWidget(domainNavigation);
}

void ActualizationNavigationSection::setBeforeNodeActionHandler(
    BeforeNodeActionHandler handler
) {
    domainNavigation->setBeforeNodeActionHandler(std::move(handler));
}

NavigationSidebar::NavigationSidebar(ApiClient &apiClient, QWidget *parent)
    : QWidget(parent) {
    setMinimumWidth(260);
    setStyleSheet("NavigationSidebar { background: palette(base); }");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *cardContainer = new QWidget(scrollArea);
    auto *cardLayout = new QVBoxLayout(cardContainer);
    cardLayout->setContentsMargins(9, 9, 9, 9);
    cardLayout->setSpacing(8);

    auto *tasksSection = new TasksNavigationSection(this);
    auto *calendarSection = new CalendarNavigationSection(this);
    auto *learningSection = new LearningNavigationSection(this);
    actualizationSection = new ActualizationNavigationSection(apiClient, this);

    cardLayout->addWidget(tasksSection);
    cardLayout->addWidget(calendarSection);
    cardLayout->addWidget(learningSection);
    cardLayout->addWidget(actualizationSection);
    cardLayout->addStretch();
    scrollArea->setWidget(cardContainer);
    layout->addWidget(scrollArea);

    connect(tasksSection, &TasksNavigationSection::openRequested,
            this, &NavigationSidebar::openTasksRequested);
    connect(calendarSection, &CalendarNavigationSection::openRequested,
            this, &NavigationSidebar::openCalendarRequested);
    connect(learningSection, &LearningNavigationSection::openRequested,
            this, &NavigationSidebar::openLearningRequested);
    connect(actualizationSection,
            &ActualizationNavigationSection::openDomainNodeRequested,
            this, &NavigationSidebar::openDomainNodeRequested);
}

void NavigationSidebar::setBeforeNodeActionHandler(
    BeforeNodeActionHandler handler
) {
    actualizationSection->setBeforeNodeActionHandler(std::move(handler));
}
