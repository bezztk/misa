#include "MainWindow.h"
#include "TaskView.h"
#include "DomainView.h"
#include "CalendarView.h"
#include "LearningView.h"

#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), apiClient(this) {
    setWindowTitle("Misa");
    resize(1600, 900);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *chromeSeparator = new QFrame(central);
    chromeSeparator->setFixedHeight(1);
    chromeSeparator->setStyleSheet("background: rgba(255, 255, 255, 0.20); border: none;");
    mainLayout->addWidget(chromeSeparator);

    auto *body = new QWidget(central);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    auto *navbar = new QWidget(body);
    navbar->setFixedWidth(48);
    auto *navLayout = new QVBoxLayout(navbar);
    navLayout->setContentsMargins(6, 6, 6, 6);
    navLayout->setSpacing(4);

    constexpr auto navButtonStyle = R"(
        QPushButton {
            background: transparent;
            border: none;
            border-radius: 6px;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 0.10);
        }
        QPushButton:checked {
            background: rgba(255, 255, 255, 0.18);
        }
        QPushButton:pressed {
            background: rgba(255, 255, 255, 0.25);
        }
    )";

    auto *workButton = new QPushButton(navbar);
    workButton->setToolTip("Arbeitsbereich");
    workButton->setFixedSize(36, 36);
    workButton->setIcon(QIcon(":/assets/icons/calendar.png"));
    workButton->setIconSize(QSize(24, 24));
    workButton->setCheckable(true);
    workButton->setAutoExclusive(true);
    workButton->setChecked(true);
    workButton->setStyleSheet(navButtonStyle);
    navLayout->addWidget(workButton, 0, Qt::AlignCenter);

    auto *learningButton = new QPushButton(navbar);
    learningButton->setToolTip("Lernen");
    learningButton->setFixedSize(36, 36);
    learningButton->setIcon(QIcon(":/assets/icons/flashcards-white.png"));
    learningButton->setIconSize(QSize(24, 24));
    learningButton->setCheckable(true);
    learningButton->setAutoExclusive(true);
    learningButton->setStyleSheet(navButtonStyle);
    navLayout->addWidget(learningButton, 0, Qt::AlignCenter);

    auto *domainButton = new QPushButton(navbar);
    domainButton->setToolTip("Verwirklichung");
    domainButton->setFixedSize(36, 36);
    domainButton->setIcon(QIcon(":/assets/icons/folder.png"));
    domainButton->setIconSize(QSize(24, 24));
    domainButton->setCheckable(true);
    domainButton->setAutoExclusive(true);
    domainButton->setStyleSheet(navButtonStyle);
    navLayout->addWidget(domainButton, 0, Qt::AlignCenter);
    navLayout->addStretch();

    auto *taskView = new TaskView(apiClient);
    auto *calendarView = new CalendarView(apiClient);
    auto *workspace = new QWidget();
    auto *workspaceLayout = new QVBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(0);

    auto *workspaceNavigation = new QWidget(workspace);
    auto *workspaceNavigationLayout = new QHBoxLayout(workspaceNavigation);
    workspaceNavigationLayout->setContentsMargins(16, 8, 16, 8);
    auto *tasksButton = new QPushButton("Aufgaben", workspaceNavigation);
    auto *calendarButton = new QPushButton("Kalender", workspaceNavigation);
    tasksButton->setCheckable(true);
    tasksButton->setAutoExclusive(true);
    tasksButton->setChecked(true);
    calendarButton->setCheckable(true);
    calendarButton->setAutoExclusive(true);
    workspaceNavigationLayout->addWidget(tasksButton);
    workspaceNavigationLayout->addWidget(calendarButton);
    workspaceNavigationLayout->addStretch();
    workspaceLayout->addWidget(workspaceNavigation);

    auto *workspaceSeparator = new QFrame(workspace);
    workspaceSeparator->setFixedHeight(1);
    workspaceSeparator->setStyleSheet(
        "background: rgba(255, 255, 255, 0.20); border: none;"
    );
    workspaceLayout->addWidget(workspaceSeparator);

    auto *workspaceStack = new QStackedWidget(workspace);
    workspaceStack->addWidget(taskView);
    workspaceStack->addWidget(calendarView);
    workspaceLayout->addWidget(workspaceStack, 1);

    connect(tasksButton, &QPushButton::clicked, this, [workspaceStack, taskView] {
        taskView->refreshTasks();
        workspaceStack->setCurrentWidget(taskView);
    });
    connect(calendarButton, &QPushButton::clicked, this, [workspaceStack, calendarView] {
        workspaceStack->setCurrentWidget(calendarView);
    });

    auto *learningView = new LearningView(apiClient);
    domainView = new DomainView(apiClient);

    connect(&apiClient, &ApiClient::metadataReceived, taskView, &TaskView::refreshMetadata);
    connect(&apiClient, &ApiClient::metadataReceived, learningView, &LearningView::refreshMetadata);
    const auto refreshSharedTasks = [this] { apiClient.getTasks(); };
    connect(&apiClient, &ApiClient::taskCreated, this, refreshSharedTasks);
    connect(&apiClient, &ApiClient::taskUpdated, this, refreshSharedTasks);
    connect(&apiClient, &ApiClient::taskDeleted, this, refreshSharedTasks);
    connect(&apiClient, &ApiClient::nodeFlashcardsChanged, this,
            [this](const int) { apiClient.getDueFlashcards(); });
    connect(&apiClient, &ApiClient::requestFailed, this,
            [this](const QString &message) {
                QMessageBox::warning(this, "Netzwerkfehler", message);
            });

    auto *stack = new QStackedWidget(body);
    stack->addWidget(workspace);
    stack->addWidget(learningView);
    stack->addWidget(domainView);
    stack->setCurrentWidget(workspace);

    bodyLayout->addWidget(navbar);
    auto *navigationSeparator = new QFrame(body);
    navigationSeparator->setFixedWidth(1);
    navigationSeparator->setStyleSheet(
        "background: rgba(255, 255, 255, 0.20); border: none;"
    );
    bodyLayout->addWidget(navigationSeparator);
    bodyLayout->addWidget(stack, 1);
    mainLayout->addWidget(body, 1);

    setCentralWidget(central);

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

    connect(workButton, &QPushButton::clicked, this, [stack, workspace, taskView] {
        taskView->refreshTasks();
        stack->setCurrentWidget(workspace);
    });
    connect(learningButton, &QPushButton::clicked, this, [stack, learningView] {
        stack->setCurrentWidget(learningView);
    });
    connect(domainButton, &QPushButton::clicked, this, [this, stack] {
        domainView->refreshTasks();
        stack->setCurrentWidget(domainView);
    });

    connect(&apiClient, &ApiClient::taskCreated, this,
            [this] { showToast("Aufgabe erstellt"); });
    connect(&apiClient, &ApiClient::taskUpdated, this,
            [this] { showToast("Aufgabe aktualisiert"); });
    connect(&apiClient, &ApiClient::calendarEntryCreated, this,
            [this] { showToast("Kalendereintrag erstellt"); });
    connect(&apiClient, &ApiClient::calendarEntryUpdated, this,
            [this] { showToast("Kalendereintrag aktualisiert"); });
    connect(&apiClient, &ApiClient::flashcardCreated, this,
            [this] { showToast("Lernkarte erstellt"); });
    connect(&apiClient, &ApiClient::flashcardUpdated, this,
            [this] { showToast("Lernkarte aktualisiert"); });
    connect(&apiClient, &ApiClient::skillCreated, this,
            [this] { showToast("Skill erstellt"); });
    connect(&apiClient, &ApiClient::skillUpdated, this,
            [this] { showToast("Skill aktualisiert"); });
    connect(&apiClient, &ApiClient::pursuitCreated, this,
            [this] { showToast("Bestrebung erstellt"); });
    connect(&apiClient, &ApiClient::pursuitUpdated, this,
            [this] { showToast("Bestrebung aktualisiert"); });
    connect(&apiClient, &ApiClient::progressionCreated, this,
            [this] { showToast("Progression erstellt"); });
    connect(&apiClient, &ApiClient::progressionUpdated, this,
            [this] { showToast("Progression aktualisiert"); });
    connect(&apiClient, &ApiClient::progressionStageCreated, this,
            [this] { showToast("Progressionsstufe erstellt"); });
    connect(&apiClient, &ApiClient::pursuitAssigned, this,
            [this] { showToast("Bestrebung hinzugefügt"); });
    connect(&apiClient, &ApiClient::domainNodeCreated, this,
            [this](const DomainNode &) { showToast("Knoten erstellt"); });
    connect(&apiClient, &ApiClient::domainNodeFieldUpdated, this,
            [this](const int, const DomainNodeField, const QJsonValue &) {
                showToast("Knoten aktualisiert");
            });
    connect(&apiClient, &ApiClient::documentCreated, this,
            [this](const Document &) { showToast("Dokument erstellt"); });

    apiClient.getMetadata();
    apiClient.getTasks();
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
    domainView->saveBeforeClose([this] {
        closeAfterSave = true;
        close();
    });
}
