#include "TaskView.h"

#include <QLabel>
#include <QVBoxLayout>

TaskView::TaskView(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    auto* label = new QLabel("Aufgabenbereich", this);
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(label);

    connect(&apiClient, &ApiClient::taskReceived, this,
        [this](const std::vector<Task>& receivedTasks) {
            tasks = receivedTasks;

            for (const Task& task : tasks) {
                qDebug() << task.name;
            }
    });

    apiClient.getTasks();
}