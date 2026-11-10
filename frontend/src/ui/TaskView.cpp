#include "TaskView.h"

#include <QLabel>
#include <QVBoxLayout>

TaskView::TaskView(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    table = new QTableWidget(this);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({
        "ID", "Name", "Status", "Priorität", "Gruppe"
    });

    layout->addWidget(table);

    connect(&apiClient, &ApiClient::taskReceived, this,
            [this](const std::vector<Task> &receivedTasks) {
                tasks = receivedTasks;

                const auto taskCount = static_cast<int>(tasks.size());

                table->setRowCount(taskCount);

                for (int i = 0; i < taskCount; i++) {
                    const Task &task = tasks[i];

                    table->setItem(i, 0, new QTableWidgetItem(QString::number(task.entity_id)));
                    table->setItem(i, 1, new QTableWidgetItem(task.name));
                    table->setItem(i, 2, new QTableWidgetItem(QString::number(task.state_id)));
                    table->setItem(i, 3, new QTableWidgetItem(QString::number(task.priority_id)));
                    table->setItem(i, 4, new QTableWidgetItem(QString::number(task.group_id)));
                }
            });

    apiClient.getTasks();
}
