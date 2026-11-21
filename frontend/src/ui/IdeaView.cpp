#include "IdeaView.h"

#include "CommentUi.h"
#include "DialogUtils.h"

#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QButtonGroup>
#include <QFont>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
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

static QString phaseColor(
    const std::vector<MetadataItem> &phases,
    const int phaseId
) {
    for (int index = 0; index < static_cast<int>(phases.size()); ++index) {
        if (phases[index].id == phaseId) {
            return phaseColor(index);
        }
    }
    return "#E5E5E5";
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
        const bool hasDescription = !idea.content.simplified().isEmpty();
        setFixedHeight(hasDescription ? 52 : 30);
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(selected
            ? "QFrame#ideaCard { background: rgba(120, 150, 210, 0.14); "
              "border: none; border-left: 2px solid #7896D2; "
              "}"
            : "QFrame#ideaCard { background: transparent; border: none; }"
              "QFrame#ideaCard:hover { background: rgba(255, 255, 255, 0.04); }");

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(10, 5, 10, 5);
        layout->setSpacing(1);

        auto *titleRow = new QWidget(this);
        titleRow->setAttribute(Qt::WA_TransparentForMouseEvents);
        auto *titleLayout = new QHBoxLayout(titleRow);
        titleLayout->setContentsMargins(0, 0, 0, 0);
        titleLayout->setSpacing(8);
        auto *title = new ElidedLabel(idea.title, titleRow);
        QFont titleFont = title->font();
        titleFont.setBold(true);
        title->setFont(titleFont);
        title->setAttribute(Qt::WA_TransparentForMouseEvents);
        titleLayout->addWidget(title, 1);
        auto *date = new QLabel(
            idea.createdAt.toLocalTime().toString("dd.MM.yyyy"), titleRow
        );
        date->setStyleSheet("color: #888888; border: none; background: transparent;");
        date->setAttribute(Qt::WA_TransparentForMouseEvents);
        titleLayout->addWidget(date);
        layout->addWidget(titleRow);

        if (hasDescription) {
            auto *description = new ElidedLabel(idea.content, this);
            description->setAttribute(Qt::WA_TransparentForMouseEvents);
            description->setStyleSheet(
                "color: #B8B8B8; border: none; background: transparent;"
            );
            layout->addWidget(description);
        }
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

    content = new QWidget(this);
    content->setFixedWidth(520);
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    outerLayout->addWidget(content);
    outerLayout->setAlignment(content, Qt::AlignHCenter);

    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(14);

    body = new QWidget(content);
    bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(24);

    auto *phaseArea = new QScrollArea(body);
    phaseArea->setFixedWidth(520);
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
                updateDetailVisibility();
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
    updateDetailVisibility();
}

void IdeaView::refreshMetadata() {
    renderIdeas();
    renderDetail();
}

void IdeaView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateDetailVisibility();
}

void IdeaView::updateDetailVisibility() {
    const Idea *idea = ideaById(selectedIdeaId);
    const bool shouldShowDetail = idea && idea->stateId == 1 && width() >= 800;
    const int contentWidth = shouldShowDetail
        ? qMin(900, width() - 40)
        : 520;
    content->setFixedWidth(contentWidth);
    content->updateGeometry();

    if (shouldShowDetail && !detail) {
        detail = new QFrame(body);
        detail->setMaximumWidth(356);
        detail->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        detailLayout = new QVBoxLayout(detail);
        detailLayout->setContentsMargins(0, 0, 0, 0);
        detailLayout->setSpacing(12);
        bodyLayout->addWidget(detail, 1);
        renderDetail();
    } else if (!shouldShowDetail && detail) {
        bodyLayout->removeWidget(detail);
        delete detail;
        detail = nullptr;
        detailLayout = nullptr;
    }
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
        auto *dot = new QLabel(track);
        dot->setFixedSize(10, 10);
        dot->move(5, 9);
        dot->setStyleSheet(
            "background: " + color + "; border: none; border-radius: 5px;"
        );
        sectionLayout->addWidget(track);

        auto *cards = new QWidget(section);
        auto *cardsLayout = new QVBoxLayout(cards);
        cardsLayout->setContentsMargins(0, 14, 0, 6);
        cardsLayout->setSpacing(0);
        auto *separator = new QFrame(cards);
        separator->setFixedHeight(1);
        separator->setStyleSheet(
            "background: " + color + "; border: none;"
        );
        cardsLayout->addWidget(separator);
        cardsLayout->addSpacing(8);

        auto *ideaList = new QWidget(cards);
        auto *ideaListLayout = new QVBoxLayout(ideaList);
        ideaListLayout->setContentsMargins(0, 0, 0, 0);
        ideaListLayout->setSpacing(4);
        cardsLayout->addWidget(ideaList);

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
            ideaListLayout->addWidget(new IdeaCard(
                *idea,
                idea->id == selectedIdeaId,
                [this, id = idea->id] { selectIdea(id); },
                [this, id = idea->id](const QPoint &position) {
                    showCardContextMenu(id, position);
                },
                ideaList
            ));
        }
        sectionLayout->addWidget(cards, 1);
        phaseLayout->addWidget(section);

        if (phaseIndex + 1
            < static_cast<int>(apiClient.metadata().ideaPhases.size())) {
            phaseLayout->addSpacing(60);
        }
    }
    phaseLayout->addStretch();
}

void IdeaView::renderDetail() {
    if (!detailLayout) {
        return;
    }
    clearLayout(detailLayout);
    Idea *idea = ideaById(selectedIdeaId);
    if (!idea || idea->stateId != 1) {
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
            if (version.fieldName == "phase_id") {
                events.push_back({.at = version.changedAt, .version = &version});
            }
        }
        std::ranges::sort(events, [](const ActivityEvent &left,
                                    const ActivityEvent &right) {
            return left.at < right.at;
        });

        QDate currentDay;
        const QLocale german(QLocale::German, QLocale::Germany);
        bool hasEventOnCurrentDay = false;
        for (const ActivityEvent &event : events) {
            const QDateTime localTime = event.at.toLocalTime();
            if (localTime.date() != currentDay) {
                if (currentDay.isValid()) {
                    activityLayout->addSpacing(18);
                }
                currentDay = localTime.date();
                auto *dayHeader = new QWidget(activityContent);
                auto *dayHeaderLayout = new QHBoxLayout(dayHeader);
                dayHeaderLayout->setContentsMargins(0, 0, 0, 8);
                dayHeaderLayout->setSpacing(10);
                auto *dayLabel = new QLabel(
                    german.toString(currentDay, "dddd, d. MMMM"), dayHeader
                );
                QFont dayFont = dayLabel->font();
                dayFont.setBold(true);
                dayLabel->setFont(dayFont);
                dayHeaderLayout->addWidget(dayLabel);
                auto *daySeparator = new QFrame(dayHeader);
                daySeparator->setFixedHeight(1);
                daySeparator->setStyleSheet(
                    "background: #505050; border: none;"
                );
                dayHeaderLayout->addWidget(daySeparator, 1);
                activityLayout->addWidget(dayHeader);
                hasEventOnCurrentDay = false;
            }

            if (hasEventOnCurrentDay) {
                activityLayout->addSpacing(16);
            }
            hasEventOnCurrentDay = true;

            QString text;
            if (event.comment) {
                QString comment = event.comment->content.toHtmlEscaped();
                comment.replace("\n", "<br>");
                text = "<img src=\":/assets/icons/speech-bubble.png\" "
                    "width=\"14\" height=\"14\">&nbsp;" + comment;
            } else if (event.version) {
                const EntityFieldVersion &version = *event.version;
                const int phaseId = version.newValue.toInt();
                text = "<img src=\":/assets/icons/edit.png\" "
                    "width=\"14\" height=\"14\">&nbsp;Phase geändert zu "
                    "<span style=\"color: "
                    + phaseColor(apiClient.metadata().ideaPhases, phaseId)
                    + "; font-weight: 600;\">"
                    + metadataName(apiClient.metadata().ideaPhases, phaseId).toHtmlEscaped()
                    + "</span>";
            }
            text += " <span style=\"color: #858585;\">· "
                + localTime.toString("HH:mm") + "</span>";

            auto *eventText = new QLabel(text, activityContent);
            eventText->setTextFormat(Qt::RichText);
            eventText->setWordWrap(true);
            eventText->setTextInteractionFlags(Qt::TextSelectableByMouse);
            activityLayout->addWidget(eventText);
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
    updateDetailVisibility();
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
    QAction *phaseAction = menu.addAction(
        QIcon(":/assets/icons/knowledge.png"), "Phase ändern"
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
    } else if (selectedAction == phaseAction) {
        openPhaseDialog(*idea);
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
    auto *stateEdit = new QComboBox(&dialog);
    for (const MetadataItem &state : apiClient.metadata().ideaStates) {
        stateEdit->addItem(state.displayName.value_or(state.name), state.id);
    }
    stateEdit->setCurrentIndex(stateEdit->findData(idea.stateId));
    layout->addRow("Titel", titleEdit);
    layout->addRow("Beschreibung", contentEdit);
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
    const int stateId = stateEdit->currentData().toInt();
    if (stateId != idea.stateId) {
        apiClient.updateIdeaField(idea.id, IdeaField::StateId, stateId);
    }
}

void IdeaView::openPhaseDialog(const Idea &idea) {
    QDialog dialog(this);
    dialog.setWindowTitle("Phase ändern");
    dialog.setMinimumSize(520, 420);
    dialog.resize(620, 600);

    auto *layout = new QVBoxLayout(&dialog);
    auto *scrollArea = new QScrollArea(&dialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *selection = new QWidget(scrollArea);
    auto *selectionLayout = new QVBoxLayout(selection);
    selectionLayout->setContentsMargins(0, 0, 0, 0);
    selectionLayout->setSpacing(14);
    auto *phaseButtons = new QButtonGroup(selection);
    phaseButtons->setExclusive(true);

    for (int index = 0;
         index < static_cast<int>(apiClient.metadata().ideaPhases.size());
         ++index) {
        const MetadataItem &phase = apiClient.metadata().ideaPhases[index];
        auto *entry = new QWidget(selection);
        auto *entryLayout = new QVBoxLayout(entry);
        entryLayout->setContentsMargins(0, 0, 0, 0);
        entryLayout->setSpacing(3);

        auto *radio = new QRadioButton(
            phase.displayName.value_or(phase.name), entry
        );
        radio->setChecked(phase.id == idea.phaseId);
        radio->setStyleSheet("color: " + phaseColor(index) + ";");
        phaseButtons->addButton(radio, phase.id);
        entryLayout->addWidget(radio);

        if (phase.description.has_value()) {
            auto *description = new QLabel(*phase.description, entry);
            description->setWordWrap(true);
            description->setContentsMargins(26, 0, 0, 0);
            entryLayout->addWidget(description);
        }
        selectionLayout->addWidget(entry);
    }
    selectionLayout->addStretch();
    scrollArea->setWidget(selection);
    layout->addWidget(scrollArea, 1);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    buttons->button(QDialogButtonBox::Save)->setText("Speichern");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    prepareDialog(dialog, 520);

    if (execDialog(dialog) == QDialog::Accepted
        && phaseButtons->checkedId() != idea.phaseId) {
        apiClient.updateIdeaField(
            idea.id,
            IdeaField::PhaseId,
            phaseButtons->checkedId()
        );
    }
}
