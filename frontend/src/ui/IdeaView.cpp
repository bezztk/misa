#include "IdeaView.h"

#include "CommentUi.h"
#include "ContextMenuStyle.h"
#include "DialogUtils.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStyleOption>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {

class ElidedLabel final : public QLabel {
public:
    ElidedLabel(const QString &text, QWidget *parent)
        : QLabel(parent), fullText(text.simplified()) {
        setTextFormat(Qt::PlainText);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        setText(fullText);
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QLabel::resizeEvent(event);
        setText(fontMetrics().elidedText(fullText, Qt::ElideRight, contentsRect().width()));
    }

private:
    QString fullText;
};

class IdeaCard final : public QAbstractButton {
public:
    explicit IdeaCard(QWidget *parent) : QAbstractButton(parent) {
        setObjectName("ideaCard");
        setCursor(Qt::PointingHandCursor);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QStyleOption option;
        option.initFrom(this);
        QPainter painter(this);
        style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
    }
};

} // namespace

IdeaView::IdeaView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
    auto *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(20, 0, 20, 0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea { background: transparent; }");
    outerLayout->addWidget(scroll);

    auto *phaseContent = new QWidget;
    phaseContent->setStyleSheet(R"(
        QLabel#ideaPhaseName[phaseId="1"] { color: #AAB8CC; }
        QLabel#ideaPhaseName[phaseId="2"] { color: #C3AED8; }
        QLabel#ideaPhaseName[phaseId="3"] { color: #D8BE91; }
        QLabel#ideaPhaseName[phaseId="5"] { color: #A8C5B0; }
        QFrame#ideaSeparator {
            background: rgba(255, 255, 255, 34);
        }
        QAbstractButton#ideaCard:hover {
            background: rgba(255, 255, 255, 18);
        }
        QLabel#ideaDescription, QLabel#ideaDate, QLabel#ideaCommentCount, QLabel#ideaContext,
        QLabel#ideaPhaseEmpty {
            color: #B8B8B8;
        }
        QToolButton#ideaActions {
            background: transparent;
            border: none;
            border-radius: 4px;
        }
        QToolButton#ideaActions:hover, QToolButton#ideaActions:focus {
            background: #414141;
        }
        QToolButton#ideaActions::menu-indicator {
            image: none;
        }
    )");
    phaseLayout = new QVBoxLayout(phaseContent);
    phaseLayout->setContentsMargins(0, 32, 0, 20);
    phaseLayout->setSpacing(28);
    scroll->setWidget(phaseContent);
    phaseContent->setAutoFillBackground(false);
    scroll->viewport()->setAutoFillBackground(false);

    connect(&apiClient, &ApiClient::ideasReceived, this,
            [this](const std::vector<Idea> &receivedIdeas) {
                ideas = receivedIdeas;
                renderIdeas();
            });
    connect(&apiClient, &ApiClient::contextsReceived, this,
            [this](const std::vector<Context> &receivedContexts) {
                contexts = receivedContexts;
                renderIdeas();
            });
    connect(&apiClient, &ApiClient::commentCreated, this,
            [this](const int entityId, const Comment &comment) {
                if (Idea *idea = ideaById(entityId)) {
                    idea->comments.push_back(comment);
                    renderIdeas();
                }
            });
    connect(&apiClient, &ApiClient::commentUpdated, this, [this](const Comment &comment) {
        if (Idea *idea = ideaById(comment.entityId)) {
            const auto existing = std::ranges::find(idea->comments, comment.id, &Comment::id);
            if (existing != idea->comments.end()) {
                *existing = comment;
            }
        }
    });
    connect(&apiClient, &ApiClient::commentDeleted, this, [this](const int id) {
        for (Idea &idea : ideas) {
            if (std::erase_if(idea.comments, [id](const Comment &comment) { return comment.id == id; })) {
                renderIdeas();
                break;
            }
        }
    });
    apiClient.getContexts();
}

void IdeaView::refreshMetadata() {
    renderIdeas();
}

void IdeaView::renderIdeas() {
    while (QLayoutItem *item = phaseLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    auto phaseFont = font();
    phaseFont.setPixelSize(20);
    phaseFont.setWeight(QFont::DemiBold);
    for (const MetadataItem &phase : apiClient.metadata().ideaPhases) {
        auto *section = new QWidget;
        auto *sectionLayout = new QVBoxLayout(section);
        sectionLayout->setContentsMargins(0, 0, 0, 0);
        sectionLayout->setSpacing(6);

        auto *name = new QLabel(phase.name, section);
        name->setObjectName("ideaPhaseName");
        name->setProperty("phaseId", phase.id);
        name->setFont(phaseFont);
        name->setContentsMargins(12, 0, 0, 0);
        sectionLayout->addWidget(name);

        auto *rows = new QVBoxLayout;
        rows->setSpacing(0);
        for (const Idea &idea : ideas) {
            if (idea.stateId != 1 || idea.phaseId != phase.id) {
                continue;
            }
            if (rows->count() > 0) {
                auto *separator = new QFrame(section);
                separator->setObjectName("ideaSeparator");
                separator->setFixedHeight(1);
                rows->addWidget(separator);
            }
            rows->addWidget(createCard(idea, section));
        }
        if (rows->count() == 0) {
            auto *empty = new QLabel(QString("Keine Idee in %1").arg(phase.name), section);
            empty->setObjectName("ideaPhaseEmpty");
            empty->setContentsMargins(12, 3, 0, 3);
            rows->addWidget(empty);
        }
        sectionLayout->addLayout(rows);
        phaseLayout->addWidget(section);
    }
    phaseLayout->addStretch();
}

QWidget *IdeaView::createCard(const Idea &idea, QWidget *parent) {
    auto *card = new IdeaCard(parent);
    card->setAccessibleName("Idee bearbeiten: " + idea.title);
    auto *row = new QHBoxLayout(card);
    row->setContentsMargins(12, 8, 8, 8);
    row->setSpacing(12);

    auto *content = new QWidget(card);
    content->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto *lines = new QVBoxLayout(content);
    lines->setContentsMargins(0, 0, 0, 0);
    lines->setSpacing(4);
    row->addWidget(content, 1);

    auto *topLine = new QHBoxLayout;
    topLine->setSpacing(12);
    lines->addLayout(topLine);
    auto *title = new ElidedLabel(idea.title, content);
    auto titleFont = title->font();
    titleFont.setPixelSize(14);
    titleFont.setWeight(QFont::Normal);
    title->setFont(titleFont);
    topLine->addWidget(title, 1);

    if (!idea.comments.empty()) {
        auto *count = new QLabel(
            QString("<img src=\":/assets/icons/speech-bubble.png\" width=\"14\" height=\"14\">"
                    "&nbsp;%1").arg(idea.comments.size()), content
        );
        count->setObjectName("ideaCommentCount");
        count->setTextFormat(Qt::RichText);
        count->setAccessibleName(QString("%1 Kommentare").arg(idea.comments.size()));
        topLine->addWidget(count);
    }

    auto *date = new QLabel(idea.createdAt.toLocalTime().toString("dd.MM.yyyy"), content);
    date->setObjectName("ideaDate");
    topLine->addWidget(date);

    const auto context = std::ranges::find(contexts, idea.contextId.value_or(0), &Context::id);
    const bool hasDescription = !idea.content.simplified().isEmpty();
    if (hasDescription || context != contexts.end()) {
        auto *bottomLine = new QHBoxLayout;
        bottomLine->setSpacing(12);
        lines->addLayout(bottomLine);
        if (hasDescription) {
            auto *description = new ElidedLabel(idea.content, content);
            description->setObjectName("ideaDescription");
            bottomLine->addWidget(description, 2);
        }
        if (context != contexts.end()) {
            auto *contextName = new ElidedLabel(context->title, content);
            contextName->setObjectName("ideaContext");
            contextName->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            if (context->color.isValid()) {
                contextName->setStyleSheet("color: " + context->color.name() + ";");
            }
            if (hasDescription) {
                contextName->setMaximumWidth(contextName->fontMetrics().horizontalAdvance(contextName->text()));
            }
            bottomLine->addWidget(contextName, 1);
        }
    }

    auto *actions = new QToolButton(card);
    actions->setObjectName("ideaActions");
    actions->setIcon(QIcon(":/assets/icons/option.png"));
    actions->setIconSize(QSize(18, 18));
    actions->setFixedSize(28, 28);
    actions->setAccessibleName("Aktionen für " + idea.title);
    actions->setPopupMode(QToolButton::InstantPopup);
    row->addWidget(actions);

    auto *menu = new QMenu(actions);
    actions->setMenu(menu);
    auto *edit = menu->addAction(QIcon(":/assets/icons/edit.png"), "Bearbeiten");
    auto *comments = menu->addAction(QIcon(":/assets/icons/speech-bubble.png"), "Kommentare anzeigen");
    menu->addSeparator();
    auto *remove = menu->addAction(contextMenuDeleteIcon(), "Löschen");
    connect(card, &QAbstractButton::clicked, edit, &QAction::trigger);
    connect(edit, &QAction::triggered, this, [this, id = idea.id] { openEditDialog(id); },
            Qt::QueuedConnection);
    connect(comments, &QAction::triggered, this, [this, id = idea.id] { openComments(id); },
            Qt::QueuedConnection);
    connect(remove, &QAction::triggered, this, [this, id = idea.id] { apiClient.deleteIdea(id); });

    card->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(card, &QWidget::customContextMenuRequested, menu, [card, menu](const QPoint &position) {
        menu->popup(card->mapToGlobal(position));
    });
    return card;
}

Idea *IdeaView::ideaById(const int id) {
    const auto idea = std::ranges::find(ideas, id, &Idea::id);
    return idea == ideas.end() ? nullptr : &*idea;
}

void IdeaView::openComments(const int id) {
    if (const Idea *idea = ideaById(id)) {
        auto comments = idea->comments;
        showCommentsDialog(this, apiClient, id, "Kommentare – " + idea->title, comments);
    }
}

void IdeaView::openCreateDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Idee erstellen");
    auto *layout = new QFormLayout(&dialog);
    auto *titleEdit = new QLineEdit(&dialog);
    auto *contentEdit = new QPlainTextEdit(&dialog);
    contentEdit->setMinimumHeight(180);
    layout->addRow("Titel", titleEdit);
    layout->addRow("Beschreibung", contentEdit);
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog
    );
    buttons->button(QDialogButtonBox::Save)->setText("Erstellen");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    prepareFormDialog(dialog, *buttons, 600);

    if (execDialog(dialog) != QDialog::Accepted
        || titleEdit->text().trimmed().isEmpty()) {
        return;
    }
    apiClient.createIdea({
        .title = titleEdit->text().trimmed(),
        .content = contentEdit->toPlainText()
    });
}

void IdeaView::openEditDialog(const int id) {
    const Idea *current = ideaById(id);
    if (!current) {
        return;
    }
    const Idea idea = *current;
    QDialog dialog(this);
    dialog.setWindowTitle("Idee bearbeiten");
    auto *layout = new QFormLayout(&dialog);
    auto *titleEdit = new QLineEdit(idea.title, &dialog);
    auto *contentEdit = new QPlainTextEdit(idea.content, &dialog);
    contentEdit->setMinimumHeight(180);
    layout->addRow("Titel", titleEdit);
    layout->addRow("Beschreibung", contentEdit);

    auto *phaseEdit = new QComboBox(&dialog);
    for (const MetadataItem &phase : apiClient.metadata().ideaPhases) {
        phaseEdit->addItem(phase.name, phase.id);
    }
    phaseEdit->setCurrentIndex(phaseEdit->findData(idea.phaseId));
    layout->addRow("Phase", phaseEdit);

    auto *contextEdit = new QComboBox(&dialog);
    contextEdit->addItem("Ohne Kontext", 0);
    for (const Context &context : contexts) {
        contextEdit->addItem(context.title, context.id);
    }
    contextEdit->setCurrentIndex(contextEdit->findData(idea.contextId.value_or(0)));
    layout->addRow("Kontext", contextEdit);

    auto *stateEdit = new QComboBox(&dialog);
    for (const MetadataItem &state : apiClient.metadata().ideaStates) {
        stateEdit->addItem(state.displayName.value_or(state.name), state.id);
    }
    stateEdit->setCurrentIndex(stateEdit->findData(idea.stateId));
    layout->addRow("Status", stateEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("Fertig");
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    prepareFormDialog(dialog, *buttons, 600);
    if (execDialog(dialog, true) != QDialog::Accepted) {
        return;
    }

    const QString title = titleEdit->text().trimmed();
    if (!title.isEmpty() && title != idea.title) {
        apiClient.updateIdeaField(id, IdeaField::Title, title);
    }
    const QString content = contentEdit->toPlainText();
    if (content != idea.content) {
        apiClient.updateIdeaField(id, IdeaField::Content, content);
    }
    const int phaseId = phaseEdit->currentData().toInt();
    if (phaseId != idea.phaseId) {
        apiClient.updateIdeaField(id, IdeaField::PhaseId, phaseId);
    }
    const int stateId = stateEdit->currentData().toInt();
    if (stateId != idea.stateId) {
        apiClient.updateIdeaField(id, IdeaField::StateId, stateId);
    }
    const int contextId = contextEdit->currentData().toInt();
    if (contextEdit->currentIndex() >= 0 && contextId != idea.contextId.value_or(0)) {
        apiClient.updateIdeaContext(id, contextId == 0 ? std::nullopt : std::optional(contextId));
    }
}
