#include "MainWindow.h"
#include "TaskView.h"

#include <QVBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QStackedWidget>

#include "LearningView.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Misa");
    resize(600, 400);

    auto *central = new QWidget(this);
    auto *mainLayout = new QHBoxLayout(central);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *navbar = new QWidget(central);
    navbar->setFixedWidth(100);

    auto *navLayout = new QVBoxLayout(navbar);

    auto *taskButton = new QPushButton("Aufgaben", navbar);
    navLayout->addWidget(taskButton);

    auto *learningButton = new QPushButton("Learning", navbar);
    navLayout->addWidget(learningButton);

    navLayout->addStretch();

    auto *stack = new QStackedWidget(central);

    auto *taskView = new TaskView();
    auto *learningView = new LearningView();

    stack->addWidget(taskView);
    stack->addWidget(learningView);

    stack->setCurrentIndex(0);

    mainLayout->addWidget(navbar);
    mainLayout->addWidget(stack, 1);

    setCentralWidget(central);

    connect(taskButton, &QPushButton::clicked, this, [stack, taskView] {
        stack->setCurrentWidget(taskView);
    });

    connect(learningButton, &QPushButton::clicked, this, [stack, learningView] {
        stack->setCurrentWidget(learningView);
    });
}
