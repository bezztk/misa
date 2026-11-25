#include "MainWindow.h"

#include "CalendarView.h"
#include "DomainView.h"
#include "IdeaView.h"
#include "LearningView.h"
#include "MediaView.h"
#include "NavigationSidebar.h"
#include "RssView.h"
#include "SourceView.h"
#include "TaskView.h"
#include "WorkspaceManager.h"

#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonValue>
#include <QLabel>
#include <QMessageBox>
#include <QPointer>
#include <QResizeEvent>
#include <QSplitter>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

static constexpr auto SIDEBAR_TOGGLE_STYLE = R"(
    QWidget#sidebarToggleRail {
        background: #1C1C1C;
        border: none;
        border-right: 1px solid #383838;
    }
    QWidget#sidebarToggleRail QToolButton {
        background: transparent;
        border: none;
    }
    QWidget#sidebarToggleRail QToolButton:hover {
        background: #303030;
    }
)";

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), apiClient(this) {
    setWindowTitle("Misa");
    resize(1600, 900);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *chromeSeparator = new QFrame(central);
    chromeSeparator->setFixedHeight(1);
    chromeSeparator->setStyleSheet(
        "background: rgba(255, 255, 255, 0.20); border: none;"
    );
    mainLayout->addWidget(chromeSeparator);

    auto *body = new QWidget(central);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    auto *sidebarToggleRail = new QWidget(body);
    sidebarToggleRail->setObjectName("sidebarToggleRail");
    sidebarToggleRail->setFixedWidth(28);
    sidebarToggleRail->setStyleSheet(SIDEBAR_TOGGLE_STYLE);
    auto *sidebarToggleLayout = new QVBoxLayout(sidebarToggleRail);
    sidebarToggleLayout->setContentsMargins(2, 6, 2, 6);
    auto *sidebarToggle = new QToolButton(sidebarToggleRail);
    sidebarToggle->setArrowType(Qt::LeftArrow);
    sidebarToggle->setToolTip("Hauptnavigation ausblenden");
    sidebarToggle->setAutoRaise(true);
    sidebarToggleLayout->addWidget(sidebarToggle, 0, Qt::AlignTop);
    sidebarToggleLayout->addStretch();
    bodyLayout->addWidget(sidebarToggleRail);

    auto *contentSplitter = new QSplitter(Qt::Horizontal, body);
    contentSplitter->setChildrenCollapsible(false);
    contentSplitter->setHandleWidth(1);
    contentSplitter->setStyleSheet(
        "QSplitter::handle { background: rgba(255, 255, 255, 0.25); }"
    );

    auto *navigation = new NavigationSidebar(apiClient, contentSplitter);
    workspaceManager = new WorkspaceManager(contentSplitter);
    contentSplitter->addWidget(navigation);
    contentSplitter->addWidget(workspaceManager);
    contentSplitter->setStretchFactor(0, 0);
    contentSplitter->setStretchFactor(1, 1);
    contentSplitter->setSizes({320, 1280});
    bodyLayout->addWidget(contentSplitter, 1);
    mainLayout->addWidget(body, 1);
    setCentralWidget(central);

    connect(sidebarToggle, &QToolButton::clicked, this,
            [navigation, contentSplitter, sidebarToggle, sidebarWidth = 320]() mutable {
                if (navigation->isVisible()) {
                    sidebarWidth = contentSplitter->sizes().first();
                    navigation->hide();
                    sidebarToggle->setArrowType(Qt::RightArrow);
                    sidebarToggle->setToolTip("Hauptnavigation einblenden");
                } else {
                    navigation->show();
                    const int totalWidth = contentSplitter->width();
                    contentSplitter->setSizes({
                        sidebarWidth,
                        qMax(0, totalWidth - sidebarWidth)
                    });
                    sidebarToggle->setArrowType(Qt::LeftArrow);
                    sidebarToggle->setToolTip("Hauptnavigation ausblenden");
                }
            });

    connect(navigation, &NavigationSidebar::openTasksRequested,
            this, &MainWindow::openTasks);
    connect(navigation, &NavigationSidebar::openCalendarRequested,
            this, &MainWindow::openCalendar);
    connect(navigation, &NavigationSidebar::openIdeasRequested,
            this, &MainWindow::openIdeas);
    connect(navigation, &NavigationSidebar::openLearningRequested,
            this, &MainWindow::openLearning);
    connect(navigation, &NavigationSidebar::openRssRequested,
            this, &MainWindow::openRss);
    connect(navigation, &NavigationSidebar::openSourcesRequested,
            this, &MainWindow::openSources);
    connect(navigation, &NavigationSidebar::createSourceRequested,
            this, [this] { openSources()->openCreateDialog(); });
    connect(navigation, &NavigationSidebar::createSourceCategoryRequested,
            this, [this] { openSources()->openCreateCategoryDialog(); });
    connect(navigation, &NavigationSidebar::openMediaRequested,
            this, &MainWindow::openMedia);
    connect(navigation, &NavigationSidebar::importMediaRequested,
            this, [this] { openMedia()->openImportDialog(); });
    connect(navigation, &NavigationSidebar::createRssFeedRequested,
            this, [this] { openRss()->openCreateFeedDialog(); });
    connect(navigation, &NavigationSidebar::createRssCategoryRequested,
            this, [this] { openRss()->openCreateCategoryDialog(); });
    connect(navigation, &NavigationSidebar::refreshAllRssRequested,
            &apiClient, &ApiClient::refreshAllRssFeeds);
    connect(navigation, &NavigationSidebar::createTaskRequested,
            this, [this] { openTasks()->openCreateDialog(); });
    connect(navigation, &NavigationSidebar::createCalendarEntryRequested,
            this, [this] { openCalendar()->openCreateDialog(); });
    connect(navigation, &NavigationSidebar::refreshCalendarRequested,
            this, [this] { openCalendar()->refresh(); });
    connect(navigation, &NavigationSidebar::createIdeaRequested,
            this, [this] { openIdeas()->openCreateDialog(); });
    connect(navigation, &NavigationSidebar::refreshLearningRequested,
            this, [this] { openLearning()->reloadFlashcards(); });
    connect(navigation, &NavigationSidebar::openDomainNodeRequested,
            this, &MainWindow::openDomainNode);
    navigation->setBeforeNodeActionHandler(
        [this](const int nodeId, std::function<void()> continuation) {
            workspaceManager->prepareView(
                "domain_node:" + QString::number(nodeId),
                std::move(continuation)
            );
        }
    );

    toastLabel = new QLabel(this);
    toastLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    toastLabel->setAlignment(Qt::AlignCenter);
    toastLabel->setStyleSheet(R"(
        QLabel {
            color: white;
            background: rgba(32, 32, 32, 0.94);
            border: 1px solid rgba(255, 255, 255, 0.18);
            border-radius: 7px;
            padding: 9px 16px;
        }
    )");
    toastLabel->hide();
    toastTimer = new QTimer(this);
    toastTimer->setSingleShot(true);
    connect(toastTimer, &QTimer::timeout, toastLabel, &QLabel::hide);

    const auto refreshSharedTasks = [this] { apiClient.getTasks(); };
    connect(&apiClient, &ApiClient::taskCreated, this, refreshSharedTasks);
    connect(&apiClient, &ApiClient::taskUpdated, this, refreshSharedTasks);
    connect(&apiClient, &ApiClient::taskDeleted, this, refreshSharedTasks);
    const auto refreshIdeas = [this] { apiClient.getIdeas(); };
    connect(&apiClient, &ApiClient::ideaCreated, this, refreshIdeas);
    connect(&apiClient, &ApiClient::ideaUpdated, this, refreshIdeas);
    connect(&apiClient, &ApiClient::ideaDeleted, this, refreshIdeas);
    const auto refreshRss = [this] {
        apiClient.getRssCategories();
        apiClient.getRssFeeds();
        apiClient.getRssEntries();
    };
    connect(&apiClient, &ApiClient::rssFeedCreated, this, refreshRss);
    connect(&apiClient, &ApiClient::rssFeedUpdated, this, refreshRss);
    connect(&apiClient, &ApiClient::rssFeedDeleted, this, refreshRss);
    connect(&apiClient, &ApiClient::rssFeedsRefreshed, this, refreshRss);
    connect(&apiClient, &ApiClient::rssFeedEntriesRead, this, refreshRss);
    connect(&apiClient, &ApiClient::rssCategoriesChanged, this, refreshRss);
    connect(&apiClient, &ApiClient::rssEntryUpdated, this, refreshRss);
    connect(&apiClient, &ApiClient::nodeFlashcardsChanged, this,
            [this](const int) { apiClient.getDueFlashcards(); });
    connect(&apiClient, &ApiClient::flashcardRated, this,
            [this] { apiClient.getDueFlashcardCount(); });
    connect(&apiClient, &ApiClient::requestFailed, this,
            [this](const QString &message) {
                QMessageBox::warning(this, "Netzwerkfehler", message);
            });

    connect(&apiClient, &ApiClient::domainNodeDeleted, this,
            [this](const int nodeId) {
                workspaceManager->closeView(
                    "domain_node:" + QString::number(nodeId)
                );
            });
    connect(&apiClient, &ApiClient::domainNodeFieldUpdated, this,
            [this](const int nodeId, const DomainNodeField field,
                   const QJsonValue &value) {
                if (field == DomainNodeField::Name) {
                    workspaceManager->setViewTitle(
                        "domain_node:" + QString::number(nodeId),
                        value.toString()
                    );
                }
            });

    connect(&apiClient, &ApiClient::taskCreated, this,
            [this] { showToast("Aufgabe erstellt"); });
    connect(&apiClient, &ApiClient::taskUpdated, this,
            [this] { showToast("Aufgabe aktualisiert"); });
    connect(&apiClient, &ApiClient::calendarEntryCreated, this,
            [this] { showToast("Kalendereintrag erstellt"); });
    connect(&apiClient, &ApiClient::calendarEntryUpdated, this,
            [this] { showToast("Kalendereintrag aktualisiert"); });
    connect(&apiClient, &ApiClient::ideaCreated, this,
            [this] { showToast("Idee erstellt"); });
    connect(&apiClient, &ApiClient::ideaUpdated, this,
            [this] { showToast("Idee aktualisiert"); });
    connect(&apiClient, &ApiClient::mediaImported, this,
            [this] { showToast("Medium importiert"); });
    connect(&apiClient, &ApiClient::flashcardCreated, this,
            [this] { showToast("Lernkarte erstellt"); });
    connect(&apiClient, &ApiClient::flashcardUpdated, this,
            [this] { showToast("Lernkarte aktualisiert"); });
    connect(&apiClient, &ApiClient::pursuitCreated, this,
            [this] { showToast("Bestrebung erstellt"); });
    connect(&apiClient, &ApiClient::pursuitUpdated, this,
            [this] { showToast("Bestrebung aktualisiert"); });
    connect(&apiClient, &ApiClient::domainNodeCreated, this,
            [this](const DomainNode &) { showToast("Knoten erstellt"); });
    connect(&apiClient, &ApiClient::domainNodeFieldUpdated, this,
            [this](const int, const DomainNodeField, const QJsonValue &) {
                showToast("Knoten aktualisiert");
            });
    connect(&apiClient, &ApiClient::documentCreated, this,
            [this](const Document &) { showToast("Aufschrieb erstellt"); });

    openTasks();
    apiClient.getMetadata();
    apiClient.getTasks();
    apiClient.getIdeas();
    apiClient.getDomainTree();
    apiClient.getDueFlashcardCount();
    apiClient.getRssCategories();
    apiClient.getRssFeeds();
    apiClient.getRssEntries();
    apiClient.refreshAllRssFeeds();
    startupComplete = true;
}

TaskView *MainWindow::openTasks() {
    bool created = false;
    auto *view = static_cast<TaskView *>(workspaceManager->openView(
        "tasks", "Aufgaben", [this, &created] {
            created = true;
            auto *view = new TaskView(apiClient);
            connect(&apiClient, &ApiClient::metadataReceived,
                    view, &TaskView::refreshMetadata);
            view->refreshMetadata();
            return view;
        }
    ));
    if (created && startupComplete) {
        apiClient.getTasks();
        apiClient.getDomainTree();
    }
    return view;
}

CalendarView *MainWindow::openCalendar() {
    return static_cast<CalendarView *>(workspaceManager->openView(
        "calendar", "Kalender", [this] {
            return new CalendarView(apiClient);
        }
    ));
}

IdeaView *MainWindow::openIdeas() {
    bool created = false;
    auto *view = static_cast<IdeaView *>(workspaceManager->openView(
        "ideas", "Ideen", [this, &created] {
            created = true;
            auto *view = new IdeaView(apiClient);
            connect(&apiClient, &ApiClient::metadataReceived,
                    view, &IdeaView::refreshMetadata);
            view->refreshMetadata();
            return view;
        }
    ));
    if (created) {
        apiClient.getIdeas();
    }
    return view;
}

LearningView *MainWindow::openLearning() {
    return static_cast<LearningView *>(workspaceManager->openView(
        "learning", "Lernkarten", [this] {
            auto *view = new LearningView(apiClient);
            connect(&apiClient, &ApiClient::metadataReceived,
                    view, &LearningView::refreshMetadata);
            view->refreshMetadata();
            return view;
        }
    ));
}

RssView *MainWindow::openRss() {
    bool created = false;
    auto *view = static_cast<RssView *>(workspaceManager->openView(
        "rss", "RSS", [this, &created] {
            created = true;
            return new RssView(apiClient);
        }
    ));
    if (created) {
        view->refresh();
    }
    return view;
}

SourceView *MainWindow::openSources() {
    return static_cast<SourceView *>(workspaceManager->openView(
        "sources", "Quellen", [this] { return new SourceView(apiClient); }
    ));
}

MediaView *MainWindow::openMedia() {
    auto *view = static_cast<MediaView *>(workspaceManager->openView(
        "media", "Filme und Serien", [this] {
            return new MediaView(apiClient);
        }
    ));
    view->showOverview();
    view->refresh();
    return view;
}

void MainWindow::openDomainNode(const int nodeId, const QString &title) {
    const QString key = "domain_node:" + QString::number(nodeId);
    auto view = std::make_shared<QPointer<DomainView>>();
    workspaceManager->openView(
        key,
        title,
        [this, nodeId, view] {
            *view = new DomainView(apiClient);
            (*view)->showNode(nodeId);
            return view->data();
        },
        [view](std::function<void()> continuation) {
            if (*view) {
                (*view)->saveBeforeClose(std::move(continuation));
            } else {
                continuation();
            }
        }
    );
}

void MainWindow::showToast(const QString &message) {
    toastLabel->setText(message);
    toastLabel->adjustSize();
    positionToast();
    toastLabel->raise();
    toastLabel->show();
    toastTimer->start(2500);
}

void MainWindow::positionToast() {
    toastLabel->move((width() - toastLabel->width()) / 2, 24);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    positionToast();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (closeAfterSave) {
        event->accept();
        return;
    }

    event->ignore();
    workspaceManager->prepareToClose([this] {
        closeAfterSave = true;
        close();
    });
}
