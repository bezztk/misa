#include "WorkspaceManager.h"

#include "TabGroup.h"
#include "WorkspaceSplitter.h"

#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QVBoxLayout>

#include <memory>

WorkspaceManager::WorkspaceManager(QWidget *parent) : QWidget(parent) {
    workspaceLayout = new QVBoxLayout(this);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(0);

    connect(
        qApp,
        &QApplication::focusChanged,
        this,
        [this](QWidget *, QWidget *current) {
            for (QWidget *widget = current; widget; widget = widget->parentWidget()) {
                if (auto *group = qobject_cast<TabGroup *>(widget)) {
                    if (focusedGroup != group) {
                        focusedGroup = group;
                        updateActiveView();
                    }
                    return;
                }
            }
        }
    );
}

QWidget *WorkspaceManager::openView(
    const QString &key,
    const QString &title,
    const QIcon &icon,
    ViewFactory factory,
    CloseHandler closeHandler
) {
    if (views.contains(key)) {
        ViewRecord &record = views[key];
        if (record.view && record.group) {
            focusedGroup = record.group;
            record.group->setCurrentView(record.view);
            return record.view;
        }
    }

    TabGroup *group = targetGroup();
    QWidget *view = factory();
    views.insert(key, ViewRecord{
        .view = view,
        .group = group,
        .title = title,
        .icon = icon,
        .closeHandler = std::move(closeHandler)
    });
    group->addView(view, title, icon);
    return view;
}

void WorkspaceManager::closeView(const QString &key) {
    if (views.contains(key) && views[key].view) {
        requestClose(views[key].view);
    }
}

void WorkspaceManager::setViewTitle(const QString &key, const QString &title) {
    if (!views.contains(key)) {
        return;
    }
    ViewRecord &record = views[key];
    record.title = title;
    if (record.group && record.view) {
        record.group->setViewTitle(record.view, title);
    }
}

void WorkspaceManager::prepareView(
    const QString &key,
    std::function<void()> continuation
) {
    if (views.contains(key) && views[key].closeHandler) {
        views[key].closeHandler(std::move(continuation));
    } else {
        continuation();
    }
}

void WorkspaceManager::prepareToClose(std::function<void()> continuation) {
    auto keys = std::make_shared<QStringList>(views.keys());
    auto next = std::make_shared<std::function<void(int)>>();
    *next = [this, keys, continuation = std::move(continuation), next](const int index) {
        if (index >= keys->size()) {
            continuation();
            return;
        }
        const QString &key = keys->at(index);
        if (!views.contains(key) || !views[key].closeHandler) {
            (*next)(index + 1);
            return;
        }
        views[key].closeHandler([next, index] { (*next)(index + 1); });
    };
    (*next)(0);
}

TabGroup *WorkspaceManager::createGroup() {
    auto *group = new TabGroup();
    groups.append(group);
    connect(group, &TabGroup::focusRequested, this,
            [this](TabGroup *selected) {
                focusedGroup = selected;
                updateActiveView();
            });
    connect(group, &TabGroup::tabCloseRequested, this,
            [this](TabGroup *, QWidget *view) { requestClose(view); });
    connect(group, &TabGroup::tabContextMenuRequested, this,
            [this](TabGroup *source, QWidget *view, const QPoint &position) {
                showTabContextMenu(source, view, position);
            });
    return group;
}

TabGroup *WorkspaceManager::targetGroup() {
    if (focusedGroup) {
        return focusedGroup;
    }
    for (const QPointer<TabGroup> &group : groups) {
        if (group) {
            focusedGroup = group;
            return group;
        }
    }

    TabGroup *group = createGroup();
    root = group;
    workspaceLayout->addWidget(group);
    focusedGroup = group;
    return group;
}

TabGroup *WorkspaceManager::splitGroup(
    TabGroup *group,
    const Qt::Orientation orientation
) {
    auto *newGroup = createGroup();
    if (auto *parentSplitter = qobject_cast<WorkspaceSplitter *>(group->parentWidget());
        parentSplitter && parentSplitter->orientation() == orientation) {
        parentSplitter->insertWidget(parentSplitter->indexOf(group) + 1, newGroup);
        parentSplitter->setStretchFactor(parentSplitter->indexOf(group), 1);
        parentSplitter->setStretchFactor(parentSplitter->indexOf(newGroup), 1);
    } else {
        auto *splitter = new WorkspaceSplitter(orientation);
        if (group == root) {
            workspaceLayout->replaceWidget(group, splitter);
            root = splitter;
            splitter->addWidget(group);
        } else {
            auto *parent = qobject_cast<WorkspaceSplitter *>(group->parentWidget());
            const int index = parent->indexOf(group);
            QWidget *replaced = parent->replaceWidget(index, splitter);
            splitter->addWidget(replaced);
        }
        splitter->addWidget(newGroup);
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 1);
    }
    focusedGroup = newGroup;
    return newGroup;
}

void WorkspaceManager::moveView(QWidget *view, TabGroup *target) {
    const QString key = keyForView(view);
    if (key.isEmpty() || !target || views[key].group == target) {
        return;
    }
    TabGroup *source = views[key].group;
    source->removeView(view);
    views[key].group = target;
    target->addView(view, views[key].title, views[key].icon);
    focusedGroup = target;
    removeEmptyGroup(source);
}

void WorkspaceManager::requestClose(QWidget *view) {
    const QString key = keyForView(view);
    if (key.isEmpty() || views[key].closing) {
        return;
    }
    views[key].closing = true;
    if (views[key].closeHandler) {
        views[key].closeHandler([this, key] { finishClose(key); });
    } else {
        finishClose(key);
    }
}

void WorkspaceManager::finishClose(const QString &key) {
    if (!views.contains(key)) {
        return;
    }
    const ViewRecord record = views.take(key);
    if (!record.view || !record.group) {
        return;
    }
    record.group->removeView(record.view);
    record.view->deleteLater();
    removeEmptyGroup(record.group);
}

void WorkspaceManager::removeEmptyGroup(TabGroup *group) {
    if (!group || !group->isEmpty()) {
        return;
    }
    groups.removeAll(group);
    if (focusedGroup == group) {
        focusedGroup = groups.isEmpty() ? nullptr : groups.first().data();
        updateActiveView();
    }
    if (group == root) {
        workspaceLayout->removeWidget(group);
        root = nullptr;
        group->deleteLater();
        return;
    }

    auto *splitter = qobject_cast<WorkspaceSplitter *>(group->parentWidget());
    group->setParent(nullptr);
    group->deleteLater();
    if (splitter->count() != 1) {
        return;
    }

    QWidget *remaining = splitter->widget(0);
    remaining->setParent(nullptr);
    if (splitter == root) {
        workspaceLayout->replaceWidget(splitter, remaining);
        root = remaining;
    } else {
        auto *parent = qobject_cast<WorkspaceSplitter *>(splitter->parentWidget());
        const int index = parent->indexOf(splitter);
        parent->replaceWidget(index, remaining);
    }
    splitter->deleteLater();
}

void WorkspaceManager::updateActiveView() {
    emit activeViewChanged(focusedGroup ? keyForView(focusedGroup->currentView()) : QString());
}

void WorkspaceManager::showTabContextMenu(
    TabGroup *group,
    QWidget *view,
    const QPoint &globalPosition
) {
    QMenu menu(this);
    QAction *closeAction = menu.addAction(
        QIcon(":/assets/icons/close.png"),
        "Schließen"
    );
    menu.addSeparator();
    QAction *splitRightAction = menu.addAction(
        QIcon(":/assets/icons/split-right.png"),
        "Rechts teilen"
    );
    QAction *splitDownAction = menu.addAction(
        QIcon(":/assets/icons/split-down.png"),
        "Unten teilen"
    );

    QMenu *moveMenu = menu.addMenu("In Tabgruppe verschieben");
    int groupNumber = 1;
    QHash<QAction *, TabGroup *> moveTargets;
    for (const QPointer<TabGroup> &target : groups) {
        if (!target || target == group) {
            ++groupNumber;
            continue;
        }
        QAction *action = moveMenu->addAction(
            "Tabgruppe " + QString::number(groupNumber)
        );
        moveTargets.insert(action, target);
        ++groupNumber;
    }
    moveMenu->setEnabled(!moveTargets.isEmpty());

    QAction *selected = menu.exec(globalPosition);
    if (selected == closeAction) {
        requestClose(view);
    } else if (selected == splitRightAction) {
        moveView(view, splitGroup(group, Qt::Horizontal));
    } else if (selected == splitDownAction) {
        moveView(view, splitGroup(group, Qt::Vertical));
    } else if (moveTargets.contains(selected)) {
        moveView(view, moveTargets[selected]);
    }
}

QString WorkspaceManager::keyForView(QWidget *view) const {
    for (auto iterator = views.constBegin(); iterator != views.constEnd(); ++iterator) {
        if (iterator->view == view) {
            return iterator.key();
        }
    }
    return {};
}
