#include "NavigationSidebar.h"

#include "DomainView.h"

#include <QColor>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

static constexpr auto NAVIGATION_STYLE = R"(
    NavigationSidebar,
    QScrollArea#navigationScrollArea,
    QWidget#navigationCardContainer {
        background: #1C1C1C;
    }
    QFrame#navigationCard {
        background: #262626;
        border: none;
        border-radius: 2px;
    }
    QFrame#navigationCard[clickable="true"]:hover,
    QWidget#navigationCardHeader[collapsible="true"]:hover {
        background: #303030;
    }
    QWidget#navigationCardHeader,
    QWidget#navigationCardText,
    QLabel#navigationCardIcon,
    QLabel#navigationCardTitle,
    QLabel#navigationCardSubtext,
    QToolButton#navigationCardExpand {
        background: transparent;
        border: none;
    }
    QLabel#navigationCardTitle {
        color: #E5E5E5;
        font-size: 14px;
        font-weight: 500;
    }
    QLabel#navigationCardSubtext {
        color: #999999;
        font-size: 11px;
    }
    QToolButton#navigationCardExpand {
        padding: 0;
    }
    QToolButton#navigationCardExpand:hover {
        background: rgba(255, 255, 255, 0.07);
        border-radius: 2px;
    }
)";

static QPixmap navigationIcon(const QString &path) {
    QPixmap pixmap = QIcon(path).pixmap(18, 18);
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), QColor("#B5B5B5"));
    return pixmap;
}

NavigationCard::NavigationCard(
    const QString &title,
    const QString &iconPath,
    const bool collapsible,
    QWidget *parent
) : QFrame(parent), collapsible(collapsible) {
    setObjectName("navigationCard");
    setProperty("clickable", !collapsible);
    setAttribute(Qt::WA_Hover);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    header = new QWidget(this);
    header->setObjectName("navigationCardHeader");
    header->setProperty("collapsible", collapsible);
    header->setCursor(Qt::PointingHandCursor);
    header->installEventFilter(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 12, 12, 12);
    headerLayout->setSpacing(10);

    auto *icon = new QLabel(header);
    icon->setObjectName("navigationCardIcon");
    icon->setFixedSize(18, 18);
    icon->setPixmap(navigationIcon(iconPath));
    icon->setAttribute(Qt::WA_TransparentForMouseEvents);
    headerLayout->addWidget(icon, 0, Qt::AlignTop);

    auto *text = new QWidget(header);
    text->setObjectName("navigationCardText");
    text->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto *textLayout = new QVBoxLayout(text);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    auto *titleLabel = new QLabel(title, text);
    titleLabel->setObjectName("navigationCardTitle");
    titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    textLayout->addWidget(titleLabel);

    subtextLabel = new QLabel(text);
    subtextLabel->setObjectName("navigationCardSubtext");
    subtextLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    subtextLabel->hide();
    textLayout->addWidget(subtextLabel);
    headerLayout->addWidget(text, 1);

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
    moduleContentLayout->setContentsMargins(12, 0, 12, 12);
    moduleContentLayout->setSpacing(6);
    layout->addWidget(content);
    content->hide();

    if (!collapsible) {
        setCursor(Qt::PointingHandCursor);
    }
}

QVBoxLayout *NavigationCard::contentLayout() const {
    content->show();
    return moduleContentLayout;
}

void NavigationCard::setSubtext(const QString &text) {
    subtextLabel->setText(text);
    subtextLabel->show();
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

TasksNavigationSection::TasksNavigationSection(
    ApiClient &apiClient,
    QWidget *parent
)
    : NavigationCard("Aufgaben", ":/assets/icons/task-white.png", false, parent) {
    setSubtext("Aufgaben werden geladen …");
    connect(&apiClient, &ApiClient::taskReceived, this,
            [this](const std::vector<Task> &tasks) {
                const int count = static_cast<int>(std::count_if(
                    tasks.begin(), tasks.end(),
                    [](const Task &task) {
                        return task.stateId == 1 || task.stateId == 2;
                    }
                ));
                if (count == 0) {
                    setSubtext("Keine offenen Aufgaben");
                } else if (count == 1) {
                    setSubtext("1 offene Aufgabe");
                } else {
                    setSubtext(QString::number(count) + " offene Aufgaben");
                }
            });
    connect(&apiClient, &ApiClient::tasksLoadFailed, this,
            [this] { setSubtext("Aufgaben konnten nicht geladen werden"); });
    connect(this, &NavigationCard::activated,
            this, &TasksNavigationSection::openRequested);
}

CalendarNavigationSection::CalendarNavigationSection(QWidget *parent)
    : NavigationCard("Kalender", ":/assets/icons/calendar.png", false, parent) {
    connect(this, &NavigationCard::activated,
            this, &CalendarNavigationSection::openRequested);
}

IdeasNavigationSection::IdeasNavigationSection(
    ApiClient &apiClient,
    QWidget *parent
) : NavigationCard(
        "Ideeninkubator", ":/assets/icons/incubator.png", false, parent
    ) {
    setSubtext("Ideen werden geladen …");
    connect(&apiClient, &ApiClient::ideasReceived, this,
            [this](const std::vector<Idea> &ideas) {
                const int count = static_cast<int>(std::count_if(
                    ideas.begin(), ideas.end(),
                    [](const Idea &idea) { return idea.stateId == 1; }
                ));
                setSubtext(QString::number(count) + " aktive Ideen");
            });
    connect(this, &NavigationCard::activated,
            this, &IdeasNavigationSection::openRequested);
}

LearningNavigationSection::LearningNavigationSection(
    ApiClient &apiClient,
    QWidget *parent
)
    : NavigationCard(
        "Lernkarten", ":/assets/icons/flashcards-white.png", false, parent
    ) {
    setSubtext("Lernkarten werden geladen …");
    connect(&apiClient, &ApiClient::dueFlashcardCountReceived, this,
            [this](const int count) {
                if (count == 0) {
                    setSubtext("Keine fälligen Lernkarten");
                } else if (count == 1) {
                    setSubtext("1 fällige Lernkarte");
                } else {
                    setSubtext(QString::number(count) + " fällige Lernkarten");
                }
            });
    connect(&apiClient, &ApiClient::dueFlashcardsLoadFailed, this,
            [this] { setSubtext("Lernkarten konnten nicht geladen werden"); });
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
    setStyleSheet(NAVIGATION_STYLE);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setObjectName("navigationScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *cardContainer = new QWidget(scrollArea);
    cardContainer->setObjectName("navigationCardContainer");
    auto *cardLayout = new QVBoxLayout(cardContainer);
    cardLayout->setContentsMargins(8, 8, 8, 8);
    cardLayout->setSpacing(8);

    auto *tasksSection = new TasksNavigationSection(apiClient, cardContainer);
    auto *calendarSection = new CalendarNavigationSection(cardContainer);
    auto *ideasSection = new IdeasNavigationSection(apiClient, cardContainer);
    auto *learningSection = new LearningNavigationSection(apiClient, cardContainer);
    actualizationSection = new ActualizationNavigationSection(
        apiClient, cardContainer
    );

    cardLayout->addWidget(tasksSection);
    cardLayout->addWidget(calendarSection);
    cardLayout->addWidget(ideasSection);
    cardLayout->addWidget(learningSection);
    cardLayout->addWidget(actualizationSection);
    cardLayout->addStretch();
    scrollArea->setWidget(cardContainer);
    layout->addWidget(scrollArea);

    connect(tasksSection, &TasksNavigationSection::openRequested,
            this, &NavigationSidebar::openTasksRequested);
    connect(calendarSection, &CalendarNavigationSection::openRequested,
            this, &NavigationSidebar::openCalendarRequested);
    connect(ideasSection, &IdeasNavigationSection::openRequested,
            this, &NavigationSidebar::openIdeasRequested);
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
