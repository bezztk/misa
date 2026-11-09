#include "MainWindow.h"
#include "TaskView.h"

#include <QVBoxLayout>
#include <QWidget>
#include <QPushButton>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Misa");
    resize(1200, 800);

    auto* central = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(central);

    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);

    auto* navbar = new QWidget(central);
    navbar->setFixedWidth(200);

    auto* navLayout = new QVBoxLayout(navbar);

    auto* taskButton = new QPushButton("Aufgaben", navbar);
    navLayout->addWidget(taskButton);
    navLayout->addStretch();

    auto* taskView = new TaskView(central);
    mainLayout->addWidget(navbar);
    mainLayout->addWidget(taskView, 1);

    setCentralWidget(central);

    connect(taskButton, &QPushButton::clicked, this, [taskView]() {
        taskView->show();
    });
}