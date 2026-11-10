#include "MainWindow.h"
#include "TaskView.h"

#include <QVBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>

#include "LearningView.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Misa");
    resize(600, 400);

    auto *central = new QWidget(this);
    auto *mainLayout = new QHBoxLayout(central);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *navbar = new QWidget(central);
    navbar->setFixedWidth(40);

    auto *navLayout = new QVBoxLayout(navbar);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(0);

    auto *taskButton = new QPushButton(navbar);
    taskButton->setFixedSize(40, 40);
    taskButton->setIcon(QIcon(":/assets/icons/clipboard.png"));
    taskButton->setIconSize(QSize(24, 24));
    navLayout->addWidget(taskButton, 0, Qt::AlignCenter);

    auto *learningButton = new QPushButton(navbar);
    learningButton->setFixedSize(40, 40);
    learningButton->setIcon(QIcon(":/assets/icons/flash-cards.png"));
    learningButton->setIconSize(QSize(24, 24));
    navLayout->addWidget(learningButton, 0, Qt::AlignCenter);

    navLayout->addStretch();

    auto *stack = new QStackedWidget(central);

    auto *taskView = new TaskView();
    auto *learningView = new LearningView();

    stack->addWidget(taskView);
    stack->addWidget(learningView);

    stack->setCurrentIndex(0);

    mainLayout->addWidget(navbar);

    auto *content = new QWidget(central);
    content->setObjectName("content");

    content->setStyleSheet(R"(
    QWidget#content {
        background-color: #40202B2D;
border: none;
border-top: 1px solid #ffffff;
border-left: 1px solid #ffffff;
border-style: solid;
border-color: #20ffffff;
    }
)");

    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(12, 12, 12, 12);
    contentLayout->setSpacing(0);
    contentLayout->addWidget(stack);

    mainLayout->addWidget(content, 1);


    setCentralWidget(central);

    connect(taskButton, &QPushButton::clicked, this, [stack, taskView] {
        stack->setCurrentWidget(taskView);
    });

    connect(learningButton, &QPushButton::clicked, this, [stack, learningView] {
        stack->setCurrentWidget(learningView);
    });
}
