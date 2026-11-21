#include "IdeaView.h"

#include "CommentUi.h"
#include "DialogUtils.h"

#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <functional>

static QString metadataName(const std::vector<MetadataItem> &items, const int id) {
    for (const MetadataItem &item : items) {
        if (item.id == id) {
            return item.displayName.value_or(item.name);
        }
    }
    return QString::number(id);
}

static QString phaseColor(const int index) {
    static const std::array colors{
        QString("#78A6D8"),
        QString("#9083D8"),
        QString("#C58BD4"),
        QString("#D7A64A"),
        QString("#6FB18A")
    };
    return colors[index];
}

static void clearLayout(QLayout *layout) {
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            delete widget;
        }
        if (QLayout *childLayout = item->layout()) {
            clearLayout(childLayout);
            delete childLayout;
        }
        delete item;
    }
}

class ElidedLabel final : public QLabel {
public:
    explicit ElidedLabel(const QString &text, QWidget *parent)
        : QLabel(parent), fullText(text.simplified()) {
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        setText(fullText);
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QLabel::resizeEvent(event);
        setText(fontMetrics().elidedText(
            fullText,
            Qt::ElideRight,
            event->size().width()
        ));
    }

private:
    QString fullText;
};

class IdeaCard final : public QFrame {
public:
    IdeaCard(
        const Idea &idea,
        const bool selected,
        std::function<void()> activate,
        std::function<void(const QPoint &)> openContextMenu,
        QWidget *parent
    ) : QFrame(parent),
        activate(std::move(activate)),
        openContextMenu(std::move(openContextMenu)) {
        setObjectName("ideaCard");
        setFixedHeight(52);
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(selected
            ? "QFrame#ideaCard { background: rgba(120, 150, 210, 0.14); "
              "border: none; border-left: 2px solid #7896D2; "
              "border-bottom: 1px solid #414141; }"
            : "QFrame#ideaCard { background: transparent; border: none; "
              "border-bottom: 1px solid #383838; }"
              "QFrame#ideaCard:hover { background: rgba(255, 255, 255, 0.04); }");

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(10, 5, 10, 5);
        layout->setSpacing(1);

        auto *title = new ElidedLabel(idea.title, this);
        QFont titleFont = title->font();
        titleFont.setBold(true);
        title->setFont(titleFont);
        title->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(title);

        auto *description = new ElidedLabel(idea.content, this);
        description->setAttribute(Qt::WA_TransparentForMouseEvents);
        description->setStyleSheet("color: #B8B8B8; border: none; background: transparent;");
        layout->addWidget(description);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            event->accept();
            return;
        }
        QFrame::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            activate();
            event->accept();
            return;
        }
        QFrame::mouseReleaseEvent(event);
    }

    void contextMenuEvent(QContextMenuEvent *event) override {
        openContextMenu(event->globalPos());
        event->accept();
    }

private:
    std::function<void()> activate;
    std::function<void(const QPoint &)> openContextMenu;
};

IdeaView::IdeaView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
    auto *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(20, 20, 20, 20);

    auto *content = new QWidget(this);
    content->setMaximumWidth(1200);
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    outerLayout->addWidget(content);
    outerLayout->setAlignment(content, Qt::AlignHCenter);

    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(14);

    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel("Ideeninkubator", content);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    title->setFont(titleFont);
    auto *createButton = new QPushButton("Neue Idee", content);
    connect(createButton, &QPushButton::clicked, this, [this] { openCreateDialog(); });
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(createButton);
    contentLayout->addLayout(headerLayout);

    auto *body = new QWidget(content);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(24);

    auto *phaseArea = new QScrollArea(body);
    phaseArea->setFixedWidth(600);
    phaseArea->setWidgetResizable(true);
    phaseArea->setFrameShape(QFrame::NoFrame);
    phaseArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *phaseContent = new QWidget(phaseArea);
    phaseLayout = new QVBoxLayout(phaseContent);
    phaseLayout->setContentsMargins(0, 0, 16, 0);
    phaseLayout->setSpacing(0);
    phaseLayout->addStretch();
    phaseArea->setWidget(phaseContent);
    bodyLayout->addWidget(phaseArea);

    auto *detail = new QFrame(body);
    detail->setMaximumWidth(600);
    detail->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    detailLayout = new QVBoxLayout(detail);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(12);
    bodyLayout->addWidget(detail, 1);
    contentLayout->addWidget(body, 1);

    connect(&apiClient, &ApiClient::ideasReceived, this,
            [this](const std::vector<Idea> &receivedIdeas) {
                ideas = receivedIdeas;
                Idea *selected = ideaById(selectedIdeaId);
                if (!selected || selected->stateId != 1) {
                    selectedIdeaId = 0;
                    versions.clear();
                    versionsLoading = false;
                } else {
                    versionsLoading = true;
                    apiClient.getEntityFieldVersions(selectedIdeaId);
                }
                renderIdeas();
                renderDetail();
            });
    connect(&apiClient, &ApiClient::entityFieldVersionsReceived, this,
            [this](const int entityId,
                   const std::vector<EntityFieldVersion> &receivedVersions) {
                if (entityId != selectedIdeaId) {
                    return;
                }
                versions = receivedVersions;
                versionsLoading = false;
                renderDetail();
            });
    connect(&apiClient, &ApiClient::entityFieldVersionsLoadFailed, this,
            [this](const int entityId) {
                if (entityId != selectedIdeaId) {
                    return;
                }
                versions.clear();
                versionsLoading = false;
                renderDetail();
            });
    connect(&apiClient, &ApiClient::commentCreated, this,
            [this](const int entityId, const Comment &comment) {
                Idea *idea = ideaById(entityId);
                if (!idea) {
                    return;
                }
                idea->comments.push_back(comment);
                if (entityId == selectedIdeaId) {
                    renderDetail();
                }
            });

    renderIdeas();
    renderDetail();
}

void IdeaView::refreshMetadata() {
    renderIdeas();
    renderDetail();
}

void IdeaView::renderIdeas() {
    clearLayout(phaseLayout);

    for (int phaseIndex = 0;
         phaseIndex < static_cast<int>(apiClient.metadata().ideaPhases.size());
         ++phaseIndex) {
        const MetadataItem &phase = apiClient.metadata().ideaPhases[phaseIndex];
        const QString color = phaseColor(phaseIndex);
        auto *section = new QWidget();
        section->setMinimumHeight(58);
        auto *sectionLayout = new QHBoxLayout(section);
        sectionLayout->setContentsMargins(0, 0, 0, 0);
        sectionLayout->setSpacing(8);

        auto *phaseLabel = new QLabel(phase.displayName.value_or(phase.name), section);
        phaseLabel->setFixedWidth(110);
        phaseLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
        QFont phaseFont = phaseLabel->font();
        phaseFont.setBold(true);
        phaseLabel->setFont(phaseFont);
        phaseLabel->setStyleSheet("color: " + color + ";");
        phaseLabel->setContentsMargins(0, 7, 0, 0);
        sectionLayout->addWidget(phaseLabel);

        auto *track = new QWidget(section);
        track->setFixedWidth(22);
        track->setStyleSheet(
            "border-left: 2px solid #505050; margin-left: 10px;"
        );
        auto *dot = new QLabel(track);
        dot->setFixedSize(10, 10);
        dot->move(5, 9);
        dot->setStyleSheet(
            "background: " + color + "; border: none; border-radius: 5px;"
        );
        sectionLayout->addWidget(track);

        auto *cards = new QWidget(section);
        auto *cardsLayout = new QVBoxLayout(cards);
        cardsLayout->setContentsMargins(0, 0, 0, 6);
        cardsLayout->setSpacing(0);

        std::vector<const Idea *> phaseIdeas;
        for (const Idea &idea : ideas) {
            if (idea.stateId == 1 && idea.phaseId == phase.id) {
                phaseIdeas.push_back(&idea);
            }
        }
        std::ranges::sort(phaseIdeas, [](const Idea *left, const Idea *right) {
            return left->createdAt > right->createdAt;
        });
        for (const Idea *idea : phaseIdeas) {
            cardsLayout->addWidget(new IdeaCard(
                *idea,
                idea->id == selectedIdeaId,
                [this, id = idea->id] { selectIdea(id); },
                [this, id = idea->id](const QPoint &position) {
                    showCardContextMenu(id, position);
                },
                cards
            ));
        }
        sectionLayout->addWidget(cards, 1);
        phaseLayout->addWidget(section);

        if (phaseIndex + 1
            < static_cast<int>(apiClient.metadata().ideaPhases.size())) {
            auto *separator = new QFrame();
            separator->setFixedHeight(1);
            separator->setStyleSheet(
                "background: " + color + "; border: none;"
            );
            phaseLayout->addWidget(separator);
        }
    }
    phaseLayout->addStretch();
}

void IdeaView::renderDetail() {
    clearLayout(detailLayout);
    Idea *idea = ideaById(selectedIdeaId);
    if (!idea || idea->stateId != 1) {
        auto *placeholder = new QLabel(
            "Idee auswählen, um die Details anzuzeigen.", this
        );
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setStyleSheet("color: #969696;");
        detailLayout->addWidget(placeholder, 1);
        return;
    }

    auto *header = new QWidget();
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    auto *title = new QLabel(idea->title, header);
    title->setWordWrap(true);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 3);
    title->setFont(titleFont);
    headerLayout->addWidget(title, 1);
    auto *createdAt = new QLabel(
        idea->createdAt.toLocalTime().toString("dd.MM.yyyy"), header
    );
    createdAt->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    createdAt->setStyleSheet("color: #929292;");
    headerLayout->addWidget(createdAt, 0, Qt::AlignTop | Qt::AlignRight);
    detailLayout->addWidget(header);

    auto *activityScroll = new QScrollArea(this);
    activityScroll->setWidgetResizable(true);
    activityScroll->setFrameShape(QFrame::NoFrame);
    auto *activityContent = new QWidget(activityScroll);
    auto *activityLayout = new QVBoxLayout(activityContent);
    activityLayout->setContentsMargins(0, 0, 4, 0);
    activityLayout->setSpacing(0);

    if (versionsLoading) {
        auto *loading = new QLabel("Verlauf wird geladen …", activityContent);
        loading->setStyleSheet("color: #969696;");
        activityLayout->addWidget(loading);
    } else {
        struct ActivityEvent {
            QDateTime at;
            const Comment *comment = nullptr;
            const EntityFieldVersion *version = nullptr;
        };
        std::vector<ActivityEvent> events;
        events.reserve(idea->comments.size() + versions.size());
        for (const Comment &comment : idea->comments) {
            events.push_back({.at = comment.createdAt, .comment = &comment});
        }
        for (const EntityFieldVersion &version : versions) {
            events.push_back({.at = version.changedAt, .version = &version});
        }
        std::ranges::sort(events, [](const ActivityEvent &left,
                                    const ActivityEvent &right) {
            return left.at < right.at;
        });

        for (const ActivityEvent &event : events) {
            const bool phaseChange = event.version
                && event.version->fieldName == "phase_id";

            auto *row = new QWidget(activityContent);
            auto *rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(8);

            auto *marker = new QWidget(row);
            marker->setFixedWidth(20);
            marker->setStyleSheet(
                "border-left: 2px solid #505050; margin-left: 9px;"
            );
            auto *dot = new QLabel(marker);
            dot->setFixedSize(9, 9);
            dot->move(5, phaseChange ? 20 : 10);
            dot->setStyleSheet(QString(
                "background: %1; border: none; border-radius: 4px;"
            ).arg(phaseChange ? "#E1A85A" : "#7896D2"));
            rowLayout->addWidget(marker);

            auto *eventFrame = new QFrame(row);
            eventFrame->setStyleSheet(phaseChange
                ? "QFrame { background: #383226; border: 1px solid #6F5A35; border-radius: 4px; }"
                : "QFrame { background: #292929; border: 1px solid #3D3D3D; border-radius: 4px; }");
            auto *eventLayout = new QVBoxLayout(eventFrame);
            eventLayout->setContentsMargins(10, 8, 10, 8);
            eventLayout->setSpacing(5);
            if (!phaseChange) {
                auto *date = new QLabel(
                    event.at.toLocalTime().toString("dd.MM.yyyy HH:mm"), eventFrame
                );
                date->setStyleSheet(
                    "color: #929292; border: none; background: transparent;"
                );
                eventLayout->addWidget(date);
            }

            if (event.comment) {
                auto *kind = new QLabel("Kommentar", eventFrame);
                QFont kindFont = kind->font();
                kindFont.setBold(true);
                kind->setFont(kindFont);
                kind->setStyleSheet("border: none; background: transparent;");
                eventLayout->addWidget(kind);
                auto *text = new QLabel(event.comment->content, eventFrame);
                text->setWordWrap(true);
                text->setTextInteractionFlags(Qt::TextSelectableByMouse);
                text->setStyleSheet("border: none; background: transparent;");
                eventLayout->addWidget(text);
            } else if (event.version) {
                const EntityFieldVersion &version = *event.version;
                if (version.fieldName == "phase_id") {
                    auto *text = new QLabel(
                        event.at.toLocalTime().toString("dd.MM.yyyy HH:mm")
                        + " · Phase: "
                        + metadataName(
                            apiClient.metadata().ideaPhases,
                            version.oldValue.toInt()
                        )
                        + " → "
                        + metadataName(
                            apiClient.metadata().ideaPhases,
                            version.newValue.toInt()
                        ),
                        eventFrame
                    );
                    QFont font = text->font();
                    font.setBold(true);
                    text->setFont(font);
                    text->setStyleSheet("border: none; background: transparent;");
                    eventLayout->addWidget(text);
                } else if (version.fieldName == "title") {
                    auto *text = new QLabel(
                        "Titel: „" + version.oldValue.toString()
                        + "“ → „" + version.newValue.toString() + "“",
                        eventFrame
                    );
                    text->setWordWrap(true);
                    text->setStyleSheet("border: none; background: transparent;");
                    eventLayout->addWidget(text);
                } else if (version.fieldName == "content") {
                    auto *toggle = new QToolButton(eventFrame);
                    toggle->setText("Beschreibung geändert");
                    toggle->setCheckable(true);
                    toggle->setArrowType(Qt::RightArrow);
                    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                    toggle->setStyleSheet("border: none; background: transparent;");
                    eventLayout->addWidget(toggle, 0, Qt::AlignLeft);

                    auto *changes = new QWidget(eventFrame);
                    auto *changesLayout = new QVBoxLayout(changes);
                    changesLayout->setContentsMargins(0, 0, 0, 0);
                    auto *oldText = new QLabel(
                        "Vorher:\n" + version.oldValue.toString(), changes
                    );
                    oldText->setWordWrap(true);
                    oldText->setTextInteractionFlags(Qt::TextSelectableByMouse);
                    auto *newText = new QLabel(
                        "Nachher:\n" + version.newValue.toString(), changes
                    );
                    newText->setWordWrap(true);
                    newText->setTextInteractionFlags(Qt::TextSelectableByMouse);
                    changesLayout->addWidget(oldText);
                    changesLayout->addWidget(newText);
                    changes->hide();
                    eventLayout->addWidget(changes);
                    connect(toggle, &QToolButton::toggled, changes,
                            [toggle, changes](const bool expanded) {
                                toggle->setArrowType(
                                    expanded ? Qt::DownArrow : Qt::RightArrow
                                );
                                changes->setVisible(expanded);
                            });
                } else if (version.fieldName == "state_id") {
                    auto *text = new QLabel(
                        "Status: "
                        + metadataName(
                            apiClient.metadata().ideaStates,
                            version.oldValue.toInt()
                        )
                        + " → "
                        + metadataName(
                            apiClient.metadata().ideaStates,
                            version.newValue.toInt()
                        ),
                        eventFrame
                    );
                    text->setStyleSheet("border: none; background: transparent;");
                    eventLayout->addWidget(text);
                }
            }
            auto *eventColumn = new QWidget(row);
            auto *eventColumnLayout = new QVBoxLayout(eventColumn);
            eventColumnLayout->setContentsMargins(0, 0, 0, 0);
            eventColumnLayout->setSpacing(0);
            if (phaseChange) {
                eventColumnLayout->addSpacing(10);
            }
            eventColumnLayout->addWidget(eventFrame);
            eventColumnLayout->addSpacing(phaseChange ? 10 : 4);
            rowLayout->addWidget(eventColumn, 1);
            activityLayout->addWidget(row);
        }

        if (events.empty()) {
            auto *empty = new QLabel("Noch keine Ereignisse vorhanden.", activityContent);
            empty->setStyleSheet("color: #969696;");
            activityLayout->addWidget(empty);
        }
    }
    activityLayout->addStretch();
    activityScroll->setWidget(activityContent);
    detailLayout->addWidget(activityScroll, 1);

    QTimer::singleShot(0, activityScroll, [activityScroll] {
        activityScroll->verticalScrollBar()->setValue(
            activityScroll->verticalScrollBar()->maximum()
        );
    });
}

void IdeaView::selectIdea(const int id) {
    if (selectedIdeaId == id) {
        return;
    }
    selectedIdeaId = id;
    versions.clear();
    versionsLoading = true;
    renderIdeas();
    renderDetail();
    apiClient.getEntityFieldVersions(id);
}

void IdeaView::showCardContextMenu(
    const int id,
    const QPoint &globalPosition
) {
    Idea *idea = ideaById(id);
    if (!idea) {
        return;
    }

    QMenu menu(this);
    QAction *editAction = menu.addAction(
        QIcon(":/assets/icons/edit.png"), "Bearbeiten"
    );
    QAction *commentsAction = menu.addAction(
        QIcon(":/assets/icons/speech-bubble.png"),
        idea->comments.empty()
            ? "Kommentare anzeigen"
            : "Kommentare anzeigen (" + QString::number(idea->comments.size()) + ")"
    );
    menu.addSeparator();
    QAction *deleteAction = menu.addAction(
        QIcon(":/assets/icons/delete.png"), "Löschen"
    );
    QAction *selectedAction = menu.exec(globalPosition);
    if (selectedAction == editAction) {
        openEditDialog(*idea);
    } else if (selectedAction == commentsAction) {
        showCommentsDialog(
            this,
            apiClient,
            idea->id,
            "Kommentare – " + idea->title,
            idea->comments
        );
        if (selectedIdeaId == idea->id) {
            renderDetail();
        }
    } else if (selectedAction == deleteAction) {
        apiClient.deleteIdea(idea->id);
    }
}

Idea *IdeaView::ideaById(const int id) {
    const auto idea = std::find_if(
        ideas.begin(), ideas.end(), [id](const Idea &item) { return item.id == id; }
    );
    return idea == ideas.end() ? nullptr : &*idea;
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
    layout->addRow(buttons);
    prepareDialog(dialog, 600);

    if (execDialog(dialog) != QDialog::Accepted
        || titleEdit->text().trimmed().isEmpty()) {
        return;
    }
    apiClient.createIdea({
        .title = titleEdit->text().trimmed(),
        .content = contentEdit->toPlainText()
    });
}

void IdeaView::openEditDialog(const Idea &idea) {
    QDialog dialog(this);
    dialog.setWindowTitle("Idee bearbeiten");
    auto *layout = new QFormLayout(&dialog);
    auto *titleEdit = new QLineEdit(idea.title, &dialog);
    auto *contentEdit = new QPlainTextEdit(idea.content, &dialog);
    contentEdit->setMinimumHeight(180);
    auto *phaseEdit = new QComboBox(&dialog);
    for (const MetadataItem &phase : apiClient.metadata().ideaPhases) {
        phaseEdit->addItem(phase.displayName.value_or(phase.name), phase.id);
    }
    phaseEdit->setCurrentIndex(phaseEdit->findData(idea.phaseId));
    auto *stateEdit = new QComboBox(&dialog);
    for (const MetadataItem &state : apiClient.metadata().ideaStates) {
        stateEdit->addItem(state.displayName.value_or(state.name), state.id);
    }
    stateEdit->setCurrentIndex(stateEdit->findData(idea.stateId));
    layout->addRow("Titel", titleEdit);
    layout->addRow("Beschreibung", contentEdit);
    layout->addRow("Phase", phaseEdit);
    layout->addRow("Status", stateEdit);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("Fertig");
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addRow(buttons);
    prepareDialog(dialog, 600);
    execDialog(dialog, true);

    const QString title = titleEdit->text().trimmed();
    if (!title.isEmpty() && title != idea.title) {
        apiClient.updateIdeaField(idea.id, IdeaField::Title, title);
    }
    const QString content = contentEdit->toPlainText();
    if (content != idea.content) {
        apiClient.updateIdeaField(idea.id, IdeaField::Content, content);
    }
    const int phaseId = phaseEdit->currentData().toInt();
    if (phaseId != idea.phaseId) {
        apiClient.updateIdeaField(idea.id, IdeaField::PhaseId, phaseId);
    }
    const int stateId = stateEdit->currentData().toInt();
    if (stateId != idea.stateId) {
        apiClient.updateIdeaField(idea.id, IdeaField::StateId, stateId);
    }
}
