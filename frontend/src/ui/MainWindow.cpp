#include "MainWindow.h"
#include "TaskView.h"
#include "DomainView.h"

#include <QVBoxLayout>
#include <QWidget>
#include <QFrame>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>

#include "LearningView.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    apiClient.getMetadata();

    setWindowTitle("Misa");
    resize(600, 400);

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

    auto *taskButton = new QPushButton(navbar);
    taskButton->setFixedSize(36,36);
    taskButton->setIcon(QIcon(":/assets/icons/task.png"));
    taskButton->setIconSize(QSize(24, 24));
    taskButton->setCheckable(true);
    taskButton->setAutoExclusive(true);
    taskButton->setChecked(true);
    taskButton->setStyleSheet(navButtonStyle);
    navLayout->addWidget(taskButton, 0, Qt::AlignCenter);

    auto *learningButton = new QPushButton(navbar);
    learningButton->setFixedSize(36,36);
    learningButton->setIcon(QIcon(":/assets/icons/flashcards-white.png"));
    learningButton->setIconSize(QSize(24, 24));
    learningButton->setCheckable(true);
    learningButton->setAutoExclusive(true);
    learningButton->setStyleSheet(navButtonStyle);
    navLayout->addWidget(learningButton, 0, Qt::AlignCenter);

    auto *domainButton = new QPushButton(navbar);
    domainButton->setToolTip("Verwirklichung");
    domainButton->setFixedSize(36,36);
    domainButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    domainButton->setIconSize(QSize(24, 24));
    domainButton->setCheckable(true);
    domainButton->setAutoExclusive(true);
    domainButton->setStyleSheet(navButtonStyle);
    navLayout->addWidget(domainButton, 0, Qt::AlignCenter);

    navLayout->addStretch();

    auto *stack = new QStackedWidget(body);

    auto *taskView = new TaskView();
    auto *learningView = new LearningView();
    auto *domainView = new DomainView();

    connect(&apiClient, &ApiClient::metadataReceived, taskView, &TaskView::renderTasks);
    connect(&apiClient, &ApiClient::metadataReceived, learningView, &LearningView::refreshMetadata);
    connect(&apiClient, &ApiClient::metadataReceived, domainView, &DomainView::renderDomains);

    stack->addWidget(taskView);
    stack->addWidget(learningView);
    stack->addWidget(domainView);

    stack->setCurrentIndex(0);

    bodyLayout->addWidget(navbar);

    auto *navigationSeparator = new QFrame(body);
    navigationSeparator->setFixedWidth(1);
    navigationSeparator->setStyleSheet("background: rgba(255, 255, 255, 0.20); border: none;");
    bodyLayout->addWidget(navigationSeparator);

    auto *content = new QWidget(body);
    content->setObjectName("content");

    content->setStyleSheet(R"(
    QWidget#content {
        background-color: #40202B2D;
        border: none;
    }
)");

    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    contentLayout->addWidget(stack);

    bodyLayout->addWidget(content, 1);
    mainLayout->addWidget(body, 1);


    setCentralWidget(central);

    connect(taskButton, &QPushButton::clicked, this, [stack, taskView] {
        stack->setCurrentWidget(taskView);
    });

    connect(learningButton, &QPushButton::clicked, this, [stack, learningView] {
        stack->setCurrentWidget(learningView);
    });

    connect(domainButton, &QPushButton::clicked, this, [stack, domainView] {
        stack->setCurrentWidget(domainView);
    });
}
