#include "CommentUi.h"

#include "ContextMenuStyle.h"

#include "../api/ApiClient.h"
#include "DialogUtils.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include <algorithm>
#include <optional>

static int indicatorWidth(const QStyleOptionViewItem &option, const int count) {
    return 14 + 4 + QFontMetrics(option.font).horizontalAdvance(QString::number(count));
}

CommentIndicatorDelegate::CommentIndicatorDelegate(
    std::function<void(int)> openComments,
    std::function<void()> indicatorPressed,
    QObject *parent
) : QStyledItemDelegate(parent),
    openComments(std::move(openComments)),
    indicatorPressed(std::move(indicatorPressed)) {}

void CommentIndicatorDelegate::paint(
    QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index
) const {
    const int count = index.data(COMMENT_COUNT_ROLE).toInt();
    if (count <= 0) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const int width = indicatorWidth(option, count);
    QStyleOptionViewItem backgroundOption(option);
    initStyleOption(&backgroundOption, index);
    backgroundOption.text.clear();
    backgroundOption.icon = {};
    QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    style->drawControl(
        QStyle::CE_ItemViewItem,
        &backgroundOption,
        painter,
        option.widget
    );

    QStyleOptionViewItem textOption(option);
    textOption.rect.setRight(option.rect.right() - width - 12);
    QStyledItemDelegate::paint(painter, textOption, index);

    const int left = option.rect.right() - width - 5;
    QIcon(":/assets/icons/speech-bubble.png").paint(
        painter,
        QRect(left, option.rect.center().y() - 7, 14, 14)
    );
    painter->save();
    painter->setPen(option.palette.color(QPalette::Text));
    painter->drawText(
        QRect(left + 18, option.rect.top(), width - 18, option.rect.height()),
        Qt::AlignLeft | Qt::AlignVCenter,
        QString::number(count)
    );
    painter->restore();
}

bool CommentIndicatorDelegate::editorEvent(
    QEvent *event,
    QAbstractItemModel *,
    const QStyleOptionViewItem &option,
    const QModelIndex &index
) {
    const int count = index.data(COMMENT_COUNT_ROLE).toInt();
    if (count <= 0
        || (event->type() != QEvent::MouseButtonPress
            && event->type() != QEvent::MouseButtonRelease)) {
        return false;
    }
    const auto *mouseEvent = static_cast<QMouseEvent *>(event);
    const int left = option.rect.right() - indicatorWidth(option, count) - 5;
    if (mouseEvent->button() != Qt::LeftButton || mouseEvent->position().x() < left) {
        return false;
    }
    if (event->type() == QEvent::MouseButtonPress) {
        if (indicatorPressed) {
            indicatorPressed();
        }
        return true;
    }
    openComments(index.data(COMMENT_ENTITY_ID_ROLE).toInt());
    return true;
}

class CommentEditDialog final : public QDialog {
public:
    CommentEditDialog(
        ApiClient &apiClient,
        const int entityId,
        const std::optional<Comment> &comment,
        QWidget *parent
    ) : QDialog(parent), apiClient(apiClient), entityId(entityId), commentId(
        comment ? std::optional<int>(comment->id) : std::nullopt
    ) {
        setWindowTitle(comment ? "Kommentar bearbeiten" : "Kommentar erstellen");
        resize(500, 260);
        auto *layout = new QVBoxLayout(this);

        editor = new QPlainTextEdit(this);
        editor->setPlaceholderText("Kommentar schreiben …");
        if (comment) {
            editor->setPlainText(comment->content);
        }
        layout->addWidget(editor, 1);

        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Save | QDialogButtonBox::Cancel,
            this
        );
        saveButton = buttons->button(QDialogButtonBox::Save);
        saveButton->setText(comment ? "Speichern" : "Erstellen");
        buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
        layout->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(editor, &QPlainTextEdit::textChanged, this, [this] {
            saveButton->setEnabled(!editor->toPlainText().trimmed().isEmpty());
        });
        connect(buttons, &QDialogButtonBox::accepted, this, [this] {
            const QString content = editor->toPlainText().trimmed();
            if (content.isEmpty()) {
                return;
            }
            saveButton->setEnabled(false);
            if (this->commentId) {
                this->apiClient.updateComment(*this->commentId, content);
            } else {
                this->apiClient.createComment(this->entityId, content);
            }
        });
        connect(&apiClient, &ApiClient::commentCreated, this,
                [this](const int createdEntityId, const Comment &) {
                    if (!this->commentId && createdEntityId == this->entityId) {
                        accept();
                    }
                });
        connect(&apiClient, &ApiClient::commentUpdated, this,
                [this](const Comment &comment) {
                    if (this->commentId == comment.id) {
                        accept();
                    }
                });
        connect(&apiClient, &ApiClient::requestFailed, this,
                [this] { saveButton->setEnabled(true); });
        saveButton->setEnabled(!editor->toPlainText().trimmed().isEmpty());
    }

private:
    ApiClient &apiClient;
    int entityId;
    std::optional<int> commentId;
    QPlainTextEdit *editor;
    QPushButton *saveButton;
};

class CommentDialog final : public QDialog {
public:
    CommentDialog(
        ApiClient &apiClient,
        const int entityId,
        const QString &title,
        const std::vector<Comment> &initialComments,
        QWidget *parent
    ) : QDialog(parent), apiClient(apiClient), entityId(entityId), values(initialComments) {
        setWindowTitle(title);
        resize(560, 520);
        auto *layout = new QVBoxLayout(this);

        list = new QListWidget(this);
        list->setWordWrap(true);
        list->setContextMenuPolicy(Qt::CustomContextMenu);
        layout->addWidget(list, 1);

        auto *actions = new QHBoxLayout();
        auto *newButton = new QPushButton("Neuer Kommentar", this);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
        buttons->button(QDialogButtonBox::Close)->setText("Schließen");
        actions->addWidget(newButton);
        actions->addStretch();
        actions->addWidget(buttons);
        layout->addLayout(actions);

        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(newButton, &QPushButton::clicked, this, [this] {
            CommentEditDialog dialog(
                this->apiClient,
                this->entityId,
                std::nullopt,
                this
            );
            prepareDialog(dialog);
            execDialog(dialog);
        });
        connect(list, &QListWidget::customContextMenuRequested, this,
                [this](const QPoint &position) {
                    QListWidgetItem *item = list->itemAt(position);
                    if (!item) {
                        return;
                    }
                    const int id = item->data(Qt::UserRole).toInt();
                    const auto comment = std::find_if(
                        values.begin(), values.end(),
                        [id](const Comment &value) { return value.id == id; }
                    );
                    if (comment == values.end()) {
                        return;
                    }

                    QMenu menu(list);
                    QAction *editAction = menu.addAction(
                        QIcon(":/assets/icons/edit.png"),
                        "Bearbeiten"
                    );
                    QAction *deleteAction = menu.addAction(
                        contextMenuDeleteIcon(),
                        "Löschen"
                    );
                    QAction *selectedAction = menu.exec(
                        list->viewport()->mapToGlobal(position)
                    );
                    if (selectedAction == editAction) {
                        CommentEditDialog dialog(
                            this->apiClient,
                            this->entityId,
                            *comment,
                            this
                        );
                        prepareDialog(dialog);
                        execDialog(dialog);
                    } else if (selectedAction == deleteAction
                               && QMessageBox::question(
                                   this,
                                   "Kommentar löschen",
                                   "Diesen Kommentar löschen?"
                               ) == QMessageBox::Yes) {
                        this->apiClient.deleteComment(id);
                    }
                });
        connect(&apiClient, &ApiClient::commentCreated, this,
                [this](const int createdEntityId, const Comment &comment) {
                    if (createdEntityId == this->entityId) {
                        values.push_back(comment);
                        refresh();
                    }
                });
        connect(&apiClient, &ApiClient::commentUpdated, this,
                [this](const Comment &comment) {
                    const auto existing = std::find_if(
                        values.begin(), values.end(),
                        [&comment](const Comment &value) { return value.id == comment.id; }
                    );
                    if (existing != values.end()) {
                        *existing = comment;
                        refresh();
                    }
                });
        connect(&apiClient, &ApiClient::commentDeleted, this,
                [this](const int id) {
                    std::erase_if(values, [id](const Comment &comment) {
                        return comment.id == id;
                    });
                    refresh();
                });

        refresh();
    }

    const std::vector<Comment> &comments() const {
        return values;
    }

private:
    void refresh() {
        list->clear();
        for (const Comment &comment : values) {
            auto *item = new QListWidgetItem(
                comment.content + "\n" + comment.createdAt.toLocalTime().toString("dd.MM.yyyy HH:mm"),
                list
            );
            item->setData(Qt::UserRole, comment.id);
        }
    }

    ApiClient &apiClient;
    int entityId;
    std::vector<Comment> values;
    QListWidget *list;
};

void showCommentsDialog(
    QWidget *parent,
    ApiClient &apiClient,
    const int entityId,
    const QString &title,
    std::vector<Comment> &comments
) {
    CommentDialog dialog(apiClient, entityId, title, comments, parent);
    prepareDialog(dialog);
    execDialog(dialog);
    comments = dialog.comments();
}
