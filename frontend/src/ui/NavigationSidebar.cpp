#include "NavigationSidebar.h"

#include "DomainView.h"

#include <QFrame>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <utility>

NavigationModuleSection::NavigationModuleSection(
    const QString &title,
    QWidget *parent
) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    headerButton = new QToolButton(this);
    headerButton->setText(title);
    headerButton->setCheckable(true);
    headerButton->setChecked(true);
    headerButton->setArrowType(Qt::DownArrow);
    headerButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    headerButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    headerButton->setStyleSheet(R"(
        QToolButton {
            border: none;
            border-bottom: 1px solid rgba(255, 255, 255, 0.14);
            padding: 8px 10px;
            text-align: left;
            font-weight: bold;
        }
        QToolButton:hover {
            background: rgba(255, 255, 255, 0.06);
        }
    )");
    layout->addWidget(headerButton);

    content = new QWidget(this);
    moduleContentLayout = new QVBoxLayout(content);
    moduleContentLayout->setContentsMargins(8, 8, 8, 8);
    moduleContentLayout->setSpacing(6);
    layout->addWidget(content, 1);

    connect(headerButton, &QToolButton::toggled, this, [this](const bool open) {
        headerButton->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
        content->setVisible(open);
    });
}

QVBoxLayout *NavigationModuleSection::contentLayout() const {
    return moduleContentLayout;
}

TasksNavigationSection::TasksNavigationSection(QWidget *parent)
    : NavigationModuleSection("Aufgaben", parent) {
    auto *openButton = new QPushButton("Aufgaben öffnen", this);
    contentLayout()->addWidget(openButton);
    connect(openButton, &QPushButton::clicked,
            this, &TasksNavigationSection::openRequested);
}

CalendarNavigationSection::CalendarNavigationSection(QWidget *parent)
    : NavigationModuleSection("Kalender", parent) {
    auto *openButton = new QPushButton("Kalender öffnen", this);
    contentLayout()->addWidget(openButton);
    connect(openButton, &QPushButton::clicked,
            this, &CalendarNavigationSection::openRequested);
}

LearningNavigationSection::LearningNavigationSection(QWidget *parent)
    : NavigationModuleSection("Lernkarten", parent) {
    auto *openButton = new QPushButton("Lernkarten öffnen", this);
    contentLayout()->addWidget(openButton);
    connect(openButton, &QPushButton::clicked,
            this, &LearningNavigationSection::openRequested);
}

ActualizationNavigationSection::ActualizationNavigationSection(
    ApiClient &apiClient,
    QWidget *parent
) : NavigationModuleSection("Verwirklichung", parent) {
    domainNavigation = new DomainView(apiClient, this);
    domainNavigation->setNodeOpenHandler(
        [this](const int nodeId, const QString &title) {
            emit openDomainNodeRequested(nodeId, title);
        }
    );
    contentLayout()->setContentsMargins(0, 0, 0, 0);
    contentLayout()->addWidget(domainNavigation, 1);
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

    auto *tasksSection = new TasksNavigationSection(this);
    auto *calendarSection = new CalendarNavigationSection(this);
    auto *learningSection = new LearningNavigationSection(this);
    actualizationSection = new ActualizationNavigationSection(apiClient, this);

    layout->addWidget(tasksSection);
    layout->addWidget(calendarSection);
    layout->addWidget(learningSection);
    layout->addWidget(actualizationSection, 1);

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
