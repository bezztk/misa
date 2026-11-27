#include "ContextView.h"

#include "DialogUtils.h"
#include "../api/ApiClient.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

ContextView::ContextView(ApiClient &apiClient, QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    auto *actions = new QHBoxLayout;
    auto *create = new QPushButton("Kontext erstellen", this);
    auto *rename = new QPushButton("Umbenennen", this);
    auto *remove = new QPushButton("Löschen", this);
    actions->addWidget(create);
    actions->addWidget(rename);
    actions->addWidget(remove);
    actions->addStretch();
    layout->addLayout(actions);

    auto *list = new QListWidget(this);
    list->setAccessibleName("Kontexte");
    layout->addWidget(list);
    rename->setEnabled(false);
    remove->setEnabled(false);

    connect(list, &QListWidget::currentItemChanged, this,
            [rename, remove](const QListWidgetItem *item) {
                rename->setEnabled(item != nullptr);
                remove->setEnabled(item != nullptr);
            });
    connect(create, &QPushButton::clicked, this, [this, &apiClient] {
        const auto title = showTextInputDialog(this, "Kontext erstellen", "Titel");
        if (title && !title->trimmed().isEmpty()) {
            apiClient.createContext(title->trimmed());
        }
    });
    connect(rename, &QPushButton::clicked, this, [this, list, &apiClient] {
        const auto *item = list->currentItem();
        const int id = item->data(Qt::UserRole).toInt();
        const auto title = showTextInputDialog(
            this, "Kontext umbenennen", "Titel", "Speichern", item->text()
        );
        if (title && !title->trimmed().isEmpty()) {
            apiClient.renameContext(id, title->trimmed());
        }
    });
    connect(remove, &QPushButton::clicked, this, [list, &apiClient] {
        apiClient.deleteContext(list->currentItem()->data(Qt::UserRole).toInt());
    });
    connect(&apiClient, &ApiClient::contextsReceived, this,
            [list](const std::vector<Context> &contexts) {
                const auto *selected = list->currentItem();
                const int selectedId = selected ? selected->data(Qt::UserRole).toInt() : 0;
                list->clear();
                for (const Context &context : contexts) {
                    auto *item = new QListWidgetItem(context.title, list);
                    item->setData(Qt::UserRole, context.id);
                    if (context.id == selectedId) {
                        list->setCurrentItem(item);
                    }
                }
            });
    apiClient.getContexts();
}
