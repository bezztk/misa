#include "NavigationSidebar.h"

#include "DomainView.h"

#include <QApplication>
#include <QColor>
#include <QContextMenuEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QMenu>
#include <QPainter>
#include <QScrollArea>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

static constexpr auto NAVIGATION_STYLE = R"(
    NavigationSidebar {
        background: #242424;
        border: none;
        border-right: 1px solid #5E5E5E;
    }
    QScrollArea#navigationScrollArea,
    QWidget#navigationCardContainer {
        background: #242424;
    }
    QFrame#navigationSectionDivider {
        background: #414141;
        border: none;
    }
    QWidget#navigationCardHeader {
        background: transparent;
        border: none;
        border-left: 2px solid transparent;
    }
    QWidget#navigationCardHeader:hover {
        background: #303030;
    }
    QWidget#navigationCardHeader[active="true"] {
        background: #3D3D3D;
        border-left-color: #AAAAAA;
    }
    QWidget#navigationCardText,
    QLabel#navigationCardIcon,
    QLabel#navigationCardTitle,
    QLabel#navigationCardSubtext,
    QToolButton#navigationCardExpand {
        background: transparent;
        border: none;
    }
    QLabel#navigationCardTitle {
        color: #F4F4F4;
        font-family: "Bahnschrift";
        font-size: 17px;
        font-weight: 500;
    }
    QLabel#navigationCardSubtext {
        color: #B8B8B8;
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

static QPixmap navigationIcon(const QIcon &icon, const int size, const QColor &color) {
    QPixmap pixmap = icon.pixmap(size, size);
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);
    return pixmap;
}

QIcon NavigationIcons::tabIcon(const QString &path) {
    return QIcon(navigationIcon(QIcon(path), 32, QColor("#B5B5B5")));
}

NavigationCard::NavigationCard(
    const QString &title,
    const QString &iconPath,
    const bool collapsible,
    QWidget *parent
) : NavigationCard(title, QIcon(iconPath), collapsible, parent) {}

NavigationCard::NavigationCard(
    const QString &title,
    const QIcon &cardIcon,
    const bool collapsible,
    QWidget *parent
) : QFrame(parent), collapsible(collapsible) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    header = new QWidget(this);
    header->setObjectName("navigationCardHeader");
    header->setFixedHeight(54);
    header->setAttribute(Qt::WA_Hover);
    header->setCursor(Qt::PointingHandCursor);
    header->installEventFilter(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(16, 0, 16, 0);
    headerLayout->setSpacing(12);

    auto *icon = new QLabel(header);
    icon->setObjectName("navigationCardIcon");
    icon->setFixedSize(26, 22);
    icon->setAlignment(Qt::AlignCenter);
    icon->setPixmap(navigationIcon(cardIcon, 22, QColor("#CCCCCC")));
    icon->setAttribute(Qt::WA_TransparentForMouseEvents);
    headerLayout->addWidget(icon, 0, Qt::AlignVCenter);

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
    headerLayout->addWidget(text, 1, Qt::AlignVCenter);

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
    moduleContentLayout->setContentsMargins(16, 0, 16, 0);
    layout->addWidget(content);
    content->hide();

    if (!collapsible) {
        setCursor(Qt::PointingHandCursor);
    }
}

void NavigationCard::setActive(const bool active) {
    header->setProperty("active", active);
    header->style()->unpolish(header);
    header->style()->polish(header);
    header->update();
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

void NavigationCard::contextMenuEvent(QContextMenuEvent *event) {
    emit contextMenuRequested(event->globalPos());
    event->accept();
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
    : NavigationCard("Aufgaben", NavigationIcons::tasks, false, parent) {
    setSubtext("Aufgaben werden geladen …");
    connect(&apiClient, &ApiClient::taskReceived, this,
            [this](const std::vector<Task> &tasks) {
                const int count = static_cast<int>(std::ranges::count_if(
                    tasks, &Task::isOpen
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
    connect(this, &NavigationCard::contextMenuRequested, this,
            [this](const QPoint &position) {
                QMenu menu(this);
                QAction *createAction = menu.addAction(
                    QIcon(":/assets/icons/add.png"),
                    "Neue Aufgabe erstellen"
                );
                if (menu.exec(position) == createAction) {
                    emit createRequested();
                }
            });
}

CalendarNavigationSection::CalendarNavigationSection(QWidget *parent)
    : NavigationCard("Kalender", NavigationIcons::calendar, false, parent) {
    connect(this, &NavigationCard::activated,
            this, &CalendarNavigationSection::openRequested);
    connect(this, &NavigationCard::contextMenuRequested, this,
            [this](const QPoint &position) {
                QMenu menu(this);
                QAction *createAction = menu.addAction(
                    QIcon(":/assets/icons/add.png"),
                    "Neuen Eintrag erstellen"
                );
                QAction *refreshAction = menu.addAction("Aktualisieren");
                QAction *selectedAction = menu.exec(position);
                if (selectedAction == createAction) {
                    emit createRequested();
                } else if (selectedAction == refreshAction) {
                    emit refreshRequested();
                }
            });
}

IdeasNavigationSection::IdeasNavigationSection(
    ApiClient &apiClient,
    QWidget *parent
) : NavigationCard(
        "Ideen", NavigationIcons::ideas, false, parent
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
    connect(this, &NavigationCard::contextMenuRequested, this,
            [this](const QPoint &position) {
                QMenu menu(this);
                QAction *createAction = menu.addAction(
                    QIcon(":/assets/icons/add.png"),
                    "Neue Idee erstellen"
                );
                if (menu.exec(position) == createAction) {
                    emit createRequested();
                }
            });
}

LearningNavigationSection::LearningNavigationSection(
    ApiClient &apiClient,
    QWidget *parent
)
    : NavigationCard(
        "Lernkarten", NavigationIcons::learning, false, parent
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
    connect(this, &NavigationCard::contextMenuRequested, this,
            [this](const QPoint &position) {
                QMenu menu(this);
                QAction *createSetAction = menu.addAction("Lernsatz erstellen");
                QAction *refreshAction = menu.addAction("Aktualisieren");
                QAction *selected = menu.exec(position);
                if (selected == createSetAction) {
                    emit createSetRequested();
                } else if (selected == refreshAction) {
                    emit refreshRequested();
                }
            });
}

RssNavigationSection::RssNavigationSection(ApiClient &apiClient, QWidget *parent)
    : NavigationCard("RSS", NavigationIcons::rss, false, parent) {
    setSubtext("Beiträge werden geladen …");
    connect(&apiClient, &ApiClient::rssEntriesReceived, this,
            [this](const std::vector<RssEntry> &entries) {
                const int unread = static_cast<int>(std::count_if(
                    entries.begin(), entries.end(),
                    [](const RssEntry &entry) { return !entry.isRead; }
                ));
                setSubtext(
                    unread == 1
                        ? "1 ungelesener Beitrag"
                        : QString::number(unread) + " ungelesene Beiträge"
                );
            });
    connect(this, &NavigationCard::activated,
            this, &RssNavigationSection::openRequested);
    connect(this, &NavigationCard::contextMenuRequested, this,
            [this](const QPoint &position) {
                QMenu menu(this);
                QAction *createFeedAction = menu.addAction("Feed abonnieren");
                QAction *refreshAction = menu.addAction("Alle Feeds aktualisieren");
                QAction *selected = menu.exec(position);
                if (selected == createFeedAction) {
                    emit createFeedRequested();
                } else if (selected == refreshAction) {
                    emit refreshAllRequested();
                }
            });
}

MediaNavigationSection::MediaNavigationSection(QWidget *parent)
    : NavigationCard("Filme und Serien",
                     NavigationIcons::media, false, parent) {
    connect(this, &NavigationCard::activated, this, &MediaNavigationSection::openRequested);
    connect(this, &NavigationCard::contextMenuRequested, this, [this](const QPoint &position) {
        QMenu menu(this);
        QAction *importAction = menu.addAction("Film oder Serie importieren");
        if (menu.exec(position) == importAction) {
            emit importRequested();
        }
    });
}

ActualizationNavigationSection::ActualizationNavigationSection(
    ApiClient &apiClient,
    QWidget *parent
) : NavigationCard(
        "Aufschriebe", NavigationIcons::notes, true, parent
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
    setFixedWidth(400);
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet(NAVIGATION_STYLE);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 1, 0);
    layout->setSpacing(0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setObjectName("navigationScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *cardContainer = new QWidget(scrollArea);
    cardContainer->setObjectName("navigationCardContainer");
    auto *cardLayout = new QVBoxLayout(cardContainer);
    cardLayout->setContentsMargins(0, 8, 0, 14);
    cardLayout->setSpacing(0);

    auto *tasksSection = new TasksNavigationSection(apiClient, cardContainer);
    auto *contextsSection = new NavigationCard(
        "Kontexte", NavigationIcons::contexts, false, cardContainer
    );
    auto *calendarSection = new CalendarNavigationSection(cardContainer);
    auto *ideasSection = new IdeasNavigationSection(apiClient, cardContainer);
    auto *learningSection = new LearningNavigationSection(apiClient, cardContainer);
    auto *rssSection = new RssNavigationSection(apiClient, cardContainer);
    auto *sourcesSection = new NavigationCard(
        "Quellen", NavigationIcons::sources, false, cardContainer
    );
    auto *mediaSection = new MediaNavigationSection(cardContainer);
    actualizationSection = new ActualizationNavigationSection(
        apiClient, cardContainer
    );
    sections = {
        {"tasks", tasksSection}, {"contexts", contextsSection},
        {"calendar", calendarSection}, {"ideas", ideasSection},
        {"learning", learningSection}, {"rss", rssSection},
        {"sources", sourcesSection}, {"media", mediaSection}
    };

    cardLayout->addWidget(tasksSection);
    cardLayout->addWidget(contextsSection);
    cardLayout->addWidget(calendarSection);
    cardLayout->addWidget(ideasSection);
    cardLayout->addWidget(learningSection);
    cardLayout->addWidget(rssSection);
    cardLayout->addWidget(sourcesSection);
    cardLayout->addWidget(mediaSection);
    auto *divider = new QFrame(cardContainer);
    divider->setObjectName("navigationSectionDivider");
    divider->setFixedHeight(1);
    cardLayout->addSpacing(6);
    cardLayout->addWidget(divider);
    cardLayout->addWidget(actualizationSection);
    cardLayout->addStretch();
    scrollArea->setWidget(cardContainer);
    layout->addWidget(scrollArea);

    connect(tasksSection, &TasksNavigationSection::openRequested,
            this, &NavigationSidebar::openTasksRequested);
    connect(contextsSection, &NavigationCard::activated,
            this, &NavigationSidebar::openContextsRequested);
    connect(contextsSection, &NavigationCard::contextMenuRequested, this,
            [this](const QPoint &position) {
                QMenu menu(this);
                const auto *create = menu.addAction(
                    QIcon(":/assets/icons/add.png"), "Kontext erstellen"
                );
                if (menu.exec(position) == create) {
                    emit createContextRequested();
                }
            });
    connect(calendarSection, &CalendarNavigationSection::openRequested,
            this, &NavigationSidebar::openCalendarRequested);
    connect(ideasSection, &IdeasNavigationSection::openRequested,
            this, &NavigationSidebar::openIdeasRequested);
    connect(learningSection, &LearningNavigationSection::openRequested,
            this, &NavigationSidebar::openLearningRequested);
    connect(learningSection, &LearningNavigationSection::createSetRequested,
            this, &NavigationSidebar::createLearningSetRequested);
    connect(rssSection, &RssNavigationSection::openRequested,
            this, &NavigationSidebar::openRssRequested);
    connect(sourcesSection, &NavigationCard::activated,
            this, &NavigationSidebar::openSourcesRequested);
    connect(sourcesSection, &NavigationCard::contextMenuRequested, this,
            [this](const QPoint &position) {
                QMenu menu(this);
                QAction *addAction = menu.addAction("Quelle hinzufügen");
                QAction *categoryAction = menu.addAction("Kategorie erstellen");
                const QAction *selected = menu.exec(position);
                if (selected == addAction) {
                    emit createSourceRequested();
                } else if (selected == categoryAction) {
                    emit createSourceCategoryRequested();
                }
            });
    connect(mediaSection, &MediaNavigationSection::openRequested,
            this, &NavigationSidebar::openMediaRequested);
    connect(mediaSection, &MediaNavigationSection::importRequested,
            this, &NavigationSidebar::importMediaRequested);
    connect(rssSection, &RssNavigationSection::createFeedRequested,
            this, &NavigationSidebar::createRssFeedRequested);
    connect(rssSection, &RssNavigationSection::refreshAllRequested,
            this, &NavigationSidebar::refreshAllRssRequested);
    connect(tasksSection, &TasksNavigationSection::createRequested,
            this, &NavigationSidebar::createTaskRequested);
    connect(calendarSection, &CalendarNavigationSection::createRequested,
            this, &NavigationSidebar::createCalendarEntryRequested);
    connect(calendarSection, &CalendarNavigationSection::refreshRequested,
            this, &NavigationSidebar::refreshCalendarRequested);
    connect(ideasSection, &IdeasNavigationSection::createRequested,
            this, &NavigationSidebar::createIdeaRequested);
    connect(learningSection, &LearningNavigationSection::refreshRequested,
            this, &NavigationSidebar::refreshLearningRequested);
    connect(actualizationSection,
            &ActualizationNavigationSection::openDomainNodeRequested,
            this, &NavigationSidebar::openDomainNodeRequested);
}

void NavigationSidebar::setActiveView(const QString &key) {
    NavigationCard *section = key.startsWith("domain_node:")
        ? actualizationSection : sections.value(key);
    if (section == activeSection) {
        return;
    }
    if (activeSection) {
        activeSection->setActive(false);
    }
    activeSection = section;
    if (activeSection) {
        activeSection->setActive(true);
    }
}

void NavigationSidebar::setBeforeNodeActionHandler(
    BeforeNodeActionHandler handler
) {
    actualizationSection->setBeforeNodeActionHandler(std::move(handler));
}
