#include "CommentUi.h"

#include "ContextMenuStyle.h"

#include "../api/ApiClient.h"
#include "DialogUtils.h"

#include <QAbstractItemModel>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <map>

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

namespace {

constexpr auto commentStyle = R"(
    QDialog#commentsDialog, QWidget#commentFeed {
        background: #202020;
    }
    QWidget#commentRow {
        background: #2B2B2B;
        border-radius: 7px;
    }
    QLabel#commentBody {
        color: #F1F1F1;
        font-size: 14px;
    }
    QLabel#commentTime, QLabel#commentDate, QLabel#commentEmpty {
        color: #B3B3B3;
    }
    QLabel#commentTime {
        font-size: 11px;
    }
    QLabel#commentDate {
        font-size: 12px;
    }
    QFrame#commentDivider {
        background: #3D3D3D;
    }
    QToolButton#commentMenu {
        color: transparent;
        background: transparent;
        border: 1px solid transparent;
        border-radius: 4px;
        font-size: 17px;
        font-weight: 600;
    }
    QToolButton#commentMenu[hovered="true"], QToolButton#commentMenu:focus {
        color: #E0E0E0;
    }
    QToolButton#commentMenu:hover {
        background: #3D3D3D;
    }
    QToolButton#commentMenu:focus {
        border-color: #929292;
    }
    QToolButton#commentMenu::menu-indicator {
        image: none;
    }
    QPlainTextEdit#commentInput {
        background: #292929;
        color: #F1F1F1;
        selection-background-color: #626262;
        selection-color: #FFFFFF;
        border: 1px solid #484848;
        border-radius: 6px;
        font-size: 14px;
    }
    QPlainTextEdit#commentInput:focus {
        border-color: #929292;
    }
    QPushButton#commentSubmit {
        background: #CBCBCB;
        color: #202020;
        border: 1px solid transparent;
        border-radius: 5px;
        padding: 7px 12px;
        font-weight: 600;
    }
    QPushButton#commentSubmit:hover {
        background: #E0E0E0;
    }
    QPushButton#commentSubmit:focus {
        border-color: #FFFFFF;
    }
    QPushButton#commentSubmit:disabled {
        background: #333333;
        color: #999999;
    }
)";

class CommentInput final : public QPlainTextEdit {
public:
    CommentInput(QWidget *parent, std::function<void()> submit)
        : QPlainTextEdit(parent), submit(std::move(submit)) {
        setObjectName("commentInput");
        setAccessibleName("Kommentar");
        setTabChangesFocus(true);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        document()->setDocumentMargin(padding);
        ensurePolished();

        const auto resizeToText = [this](const QSizeF &size) {
            const int lines = std::clamp(static_cast<int>(size.height()), 1, 5);
            setFixedHeight(lines * fontMetrics().lineSpacing() + 2 * (padding + frameWidth()) + 2);
        };
        auto *documentLayout = document()->documentLayout();
        connect(documentLayout, &QAbstractTextDocumentLayout::documentSizeChanged, this, resizeToText);
        resizeToText(documentLayout->documentSize());
    }

    QString content() const {
        return toPlainText().trimmed();
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            const auto modifiers = event->modifiers() & (
                Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier
            );
            if (!modifiers) {
                submit();
                event->accept();
                return;
            }
            if (modifiers == Qt::ShiftModifier && !isReadOnly()) {
                insertPlainText("\n");
                ensureCursorVisible();
                event->accept();
                return;
            }
        }
        QPlainTextEdit::keyPressEvent(event);
    }

private:
    static constexpr int padding = 9;
    std::function<void()> submit;
};

QFrame *commentDivider(QWidget *parent) {
    auto *line = new QFrame(parent);
    line->setObjectName("commentDivider");
    line->setFixedHeight(1);
    return line;
}

class CommentRow final : public QWidget {
public:
    CommentRow(const Comment &comment, ApiClient &apiClient, QDialog &dialog, QWidget *parent)
        : QWidget(parent), apiClient(apiClient), id(comment.id) {
        setObjectName("commentRow");
        setAttribute(Qt::WA_StyledBackground);

        auto *row = new QHBoxLayout(this);
        row->setContentsMargins(12, 8, 8, 8);
        row->setSpacing(8);
        auto *content = new QWidget(this);
        contentLayout = new QVBoxLayout(content);
        contentLayout->setContentsMargins(0, 2, 0, 0);
        contentLayout->setSpacing(5);
        row->addWidget(content, 1);

        body = new QLabel(comment.content, content);
        body->setObjectName("commentBody");
        body->setTextFormat(Qt::PlainText);
        body->setWordWrap(true);
        body->setTextInteractionFlags(Qt::TextSelectableByMouse);
        body->setContextMenuPolicy(Qt::NoContextMenu);
        auto bodyPolicy = body->sizePolicy();
        bodyPolicy.setHorizontalPolicy(QSizePolicy::Ignored);
        body->setSizePolicy(bodyPolicy);
        contentLayout->addWidget(body);

        auto *time = new QLabel(comment.createdAt.toLocalTime().toString("HH:mm"), content);
        time->setObjectName("commentTime");
        contentLayout->addWidget(time);

        options = new QToolButton(this);
        options->setObjectName("commentMenu");
        options->setText("…");
        options->setAccessibleName("Kommentaraktionen");
        options->setToolButtonStyle(Qt::ToolButtonTextOnly);
        options->setFocusPolicy(Qt::StrongFocus);
        options->setFixedSize(30, 27);
        options->setPopupMode(QToolButton::InstantPopup);
        row->addWidget(options, 0, Qt::AlignTop);

        auto *menu = new QMenu(options);
        options->setMenu(menu);
        auto *edit = menu->addAction(QIcon(":/assets/icons/edit.png"), "Bearbeiten");
        auto *remove = menu->addAction(contextMenuDeleteIcon(), "Löschen");
        connect(edit, &QAction::triggered, this, [this] { beginEdit(); });
        connect(remove, &QAction::triggered, this, [&apiClient, &dialog, id = comment.id] {
            if (QMessageBox::question(&dialog, "Kommentar löschen", "Diesen Kommentar löschen?")
                == QMessageBox::Yes) {
                apiClient.deleteComment(id);
            }
        });
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QWidget::customContextMenuRequested, menu, [this, menu](const QPoint &position) {
            if (!editPanel) {
                menu->popup(mapToGlobal(position));
            }
        });
    }

    void updateComment(const Comment &comment) {
        const bool saved = editPanel && editor->isReadOnly();
        body->setText(comment.content);
        if (saved) {
            finishEdit();
        }
    }

protected:
    void enterEvent(QEnterEvent *event) override {
        QWidget::enterEvent(event);
        setMenuHovered(true);
    }

    void leaveEvent(QEvent *event) override {
        QWidget::leaveEvent(event);
        setMenuHovered(false);
    }

private:
    void setMenuHovered(const bool hovered) {
        options->setProperty("hovered", hovered);
        options->style()->unpolish(options);
        options->style()->polish(options);
        options->update();
    }

    void beginEdit() {
        editPanel = new QWidget(this);
        auto *layout = new QVBoxLayout(editPanel);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);

        editor = new CommentInput(editPanel, [this] { saveEdit(); });
        editor->setAccessibleName("Kommentar bearbeiten");
        editor->setPlainText(body->text());
        layout->addWidget(editor);

        auto *actions = new QHBoxLayout;
        actions->addStretch();
        cancelButton = new QPushButton("Abbrechen", editPanel);
        cancelButton->setAutoDefault(false);
        actions->addWidget(cancelButton);
        saveButton = new QPushButton("Speichern", editPanel);
        saveButton->setObjectName("commentSubmit");
        saveButton->setAutoDefault(false);
        saveButton->setEnabled(false);
        actions->addWidget(saveButton);
        layout->addLayout(actions);

        connect(cancelButton, &QPushButton::clicked, this, [this] { finishEdit(); });
        connect(saveButton, &QPushButton::clicked, this, [this] { saveEdit(); });
        connect(editor, &QPlainTextEdit::textChanged, this, [this] { updateSaveButton(); });

        contentLayout->insertWidget(0, editPanel);
        body->hide();
        options->hide();
        editor->setFocus();
        editor->moveCursor(QTextCursor::End);
    }

    void finishEdit() {
        editPanel->hide();
        editPanel->deleteLater();
        editPanel = nullptr;
        body->show();
        options->show();
        options->setFocus();
    }

    void updateSaveButton() {
        const QString content = editor->content();
        saveButton->setEnabled(!editor->isReadOnly() && !content.isEmpty() && content != body->text());
    }

    void saveEdit() {
        if (!saveButton->isEnabled()) {
            return;
        }
        editor->setReadOnly(true);
        saveButton->setEnabled(false);
        cancelButton->setEnabled(false);

        auto *reply = apiClient.updateComment(id, editor->content());
        connect(reply, &QNetworkReply::finished, editPanel, [this, panel = editPanel] {
            if (editPanel == panel) {
                editor->setReadOnly(false);
                cancelButton->setEnabled(true);
                updateSaveButton();
            }
        });
    }

    ApiClient &apiClient;
    int id;
    QVBoxLayout *contentLayout;
    QLabel *body;
    QToolButton *options;
    QWidget *editPanel = nullptr;
    CommentInput *editor;
    QPushButton *saveButton;
    QPushButton *cancelButton;
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
        setObjectName("commentsDialog");
        setStyleSheet(commentStyle);
        setWindowTitle(title);
        resize(560, 520);
        auto *layout = new QVBoxLayout(this);

        history = new QScrollArea(this);
        history->setFrameShape(QFrame::NoFrame);
        history->setWidgetResizable(true);
        history->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        feed = new QWidget(history);
        feed->setObjectName("commentFeed");
        feedLayout = new QVBoxLayout(feed);
        feedLayout->setContentsMargins(0, 0, 6, 0);
        feedLayout->setSpacing(14);

        empty = new QLabel("Noch keine Kommentare", feed);
        empty->setObjectName("commentEmpty");
        empty->setAlignment(Qt::AlignCenter);
        feedLayout->addWidget(empty, 1);
        feedLayout->addStretch();
        history->setWidget(feed);
        layout->addWidget(history, 1);

        layout->addWidget(commentDivider(this));
        auto *composer = new QHBoxLayout;
        composer->setSpacing(8);
        input = new CommentInput(this, [this] { send(); });
        input->setPlaceholderText("Kommentar schreiben …");
        composer->addWidget(input, 1, Qt::AlignBottom);
        sendButton = new QPushButton("Senden", this);
        sendButton->setObjectName("commentSubmit");
        sendButton->setToolTip("Enter zum Senden, Umschalt+Enter für einen Zeilenumbruch");
        sendButton->setAutoDefault(false);
        sendButton->setEnabled(false);
        composer->addWidget(sendButton, 0, Qt::AlignBottom);
        layout->addLayout(composer);
        connect(sendButton, &QPushButton::clicked, this, [this] { send(); });
        connect(input, &QPlainTextEdit::textChanged, this, [this] { updateSendButton(); });

        for (const Comment &comment : values) {
            addComment(comment);
        }
        empty->setVisible(values.empty());

        connect(&apiClient, &ApiClient::commentCreated, this,
                [this](const int createdEntityId, const Comment &comment) {
                    if (createdEntityId == this->entityId) {
                        values.push_back(comment);
                        addComment(comment);
                        empty->hide();
                        if (input->isReadOnly()) {
                            input->clear();
                        }
                        scrollToLatest();
                    }
                });
        connect(&apiClient, &ApiClient::commentUpdated, this, [this](const Comment &comment) {
            const auto existing = std::ranges::find(values, comment.id, &Comment::id);
            if (existing != values.end()) {
                *existing = comment;
                rows.at(comment.id)->updateComment(comment);
            }
        });
        connect(&apiClient, &ApiClient::commentDeleted, this, [this](const int id) {
            const auto existing = std::ranges::find(values, id, &Comment::id);
            if (existing != values.end()) {
                removeComment(*existing);
                values.erase(existing);
                empty->setVisible(values.empty());
            }
        });
    }

    const std::vector<Comment> &comments() const {
        return values;
    }

protected:
    void showEvent(QShowEvent *event) override {
        QDialog::showEvent(event);
        input->setFocus();
        scrollToLatest();
    }

private:
    QWidget *addDay(const QDate date) {
        auto *day = new QWidget(feed);
        auto *layout = new QVBoxLayout(day);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);

        auto *heading = new QHBoxLayout;
        heading->setSpacing(10);
        const QDate today = QDate::currentDate();
        const QLocale german(QLocale::German, QLocale::Germany);
        const QString title = date == today ? "Heute"
            : german.toString(date, date.year() == today.year() ? "d. MMMM" : "d. MMMM yyyy");
        auto *label = new QLabel(title, day);
        label->setObjectName("commentDate");
        heading->addWidget(commentDivider(day), 1);
        heading->addWidget(label);
        heading->addWidget(commentDivider(day), 1);
        layout->addLayout(heading);

        feedLayout->insertWidget(feedLayout->count() - 1, day);
        return day;
    }

    void addComment(const Comment &comment) {
        const QDate date = comment.createdAt.toLocalTime().date();
        auto [day, inserted] = days.try_emplace(date);
        if (inserted) {
            day->second = addDay(date);
        }
        auto *row = new CommentRow(comment, apiClient, *this, day->second);
        day->second->layout()->addWidget(row);
        rows.emplace(comment.id, row);
    }

    void removeComment(const Comment &comment) {
        const auto row = rows.find(comment.id);
        auto *day = row->second->parentWidget();
        day->layout()->removeWidget(row->second);
        row->second->hide();
        row->second->deleteLater();
        rows.erase(row);
        if (day->layout()->count() == 1) {
            day->hide();
            day->deleteLater();
            days.erase(comment.createdAt.toLocalTime().date());
        }
    }

    void updateSendButton() {
        sendButton->setEnabled(!input->isReadOnly() && !input->content().isEmpty());
    }

    void send() {
        if (!sendButton->isEnabled()) {
            return;
        }
        input->setReadOnly(true);
        sendButton->setEnabled(false);
        input->setFocus();

        auto *reply = apiClient.createComment(entityId, input->content());
        connect(reply, &QNetworkReply::finished, this, [this] {
            input->setReadOnly(false);
            updateSendButton();
        });
    }

    void scrollToLatest() {
        QTimer::singleShot(0, this, [this] {
            auto *bar = history->verticalScrollBar();
            bar->setValue(bar->maximum());
        });
    }

    ApiClient &apiClient;
    int entityId;
    std::vector<Comment> values;
    std::map<QDate, QWidget *> days;
    std::map<int, CommentRow *> rows;
    QScrollArea *history;
    QWidget *feed;
    QVBoxLayout *feedLayout;
    QLabel *empty;
    CommentInput *input;
    QPushButton *sendButton;
};

} // namespace

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
