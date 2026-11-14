#include "DomainView.h"
#include "../models/AppState.h"

#include <QFrame>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QSplitter>
#include <QStandardItemModel>
#include <QStyle>
#include <QTableView>
#include <QTextBlock>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

#include <functional>

static constexpr int DOMAIN_ID_ROLE = Qt::UserRole + 1;
static constexpr int HAS_DOCUMENT_ROLE = Qt::UserRole + 2;
static constexpr int FLASHCARD_COUNT_ROLE = Qt::UserRole + 3;
static constexpr int COGNITIVE_LEVEL_ROLE = Qt::UserRole + 4;
static constexpr int COMPLETENESS_LEVEL_ROLE = Qt::UserRole + 5;
static constexpr int FLASHCARD_STATUS_ROLE = Qt::UserRole + 6;
static constexpr int VERIFICATION_STATUS_ROLE = Qt::UserRole + 7;
static constexpr int VERIFICATION_LEVEL_ROLE = Qt::UserRole + 8;
static constexpr int NODE_NAME_ROLE = Qt::UserRole + 9;
static constexpr int SKILL_COUNT_ROLE = Qt::UserRole + 10;
static constexpr int SKILL_NAME_ROLE = Qt::UserRole + 1;
static constexpr int PSYCHOMOTOR_LEVEL_ROLE = Qt::UserRole + 2;

static QString metadataName(const MetadataItem &item) {
    return item.display_name.value_or(item.name);
}

static QString skillText(const Skill &skill) {
    QString text = skill.name;
    if (skill.psychomotorLevelId.has_value()) {
        for (const MetadataItem &level : AppState::metadata.psychomotor_levels) {
            if (level.id == *skill.psychomotorLevelId) {
                text += "  ·  " + metadataName(level);
                break;
            }
        }
    }
    return text;
}

static void fillMetadataCombo(QComboBox *combo, const std::vector<MetadataItem> &items) {
    combo->clear();
    combo->addItem("—", QVariant());
    for (const MetadataItem &item : items) {
        combo->addItem(metadataName(item), item.id);
    }
}

static void selectMetadata(QComboBox *combo, const std::optional<int> id) {
    combo->setCurrentIndex(id.has_value() ? combo->findData(*id) : 0);
}

static std::optional<int> selectedMetadata(const QComboBox *combo) {
    return combo->currentData().isValid()
        ? std::optional<int>(combo->currentData().toInt())
        : std::nullopt;
}

static void showMetadata(
    QLabel *label,
    const QString &title,
    const std::vector<MetadataItem> &items,
    const std::optional<int> id
) {
    QString value = "—";
    for (const MetadataItem &item : items) {
        if (id.has_value() && item.id == *id) {
            value = metadataName(item);
            break;
        }
    }
    label->setText(title + ": " + value);
}

static void setOptionalRole(
    QStandardItem *item,
    const int role,
    const std::optional<int> value
) {
    item->setData(value.has_value() ? QVariant(*value) : QVariant(), role);
}

static std::optional<int> optionalRole(const QModelIndex &index, const int role) {
    return index.data(role).isValid()
        ? std::optional<int>(index.data(role).toInt())
        : std::nullopt;
}

static void setDomainNodeRoles(QStandardItem *item, const DomainNode &node) {
    item->setData(node.name, NODE_NAME_ROLE);
    item->setData(node.flashcardCount, FLASHCARD_COUNT_ROLE);
    item->setData(node.skillCount, SKILL_COUNT_ROLE);
    setOptionalRole(item, COGNITIVE_LEVEL_ROLE, node.cognitiveLevelId);
    setOptionalRole(item, COMPLETENESS_LEVEL_ROLE, node.completenessLevelId);
    setOptionalRole(item, FLASHCARD_STATUS_ROLE, node.flashcardStatusId);
    setOptionalRole(item, VERIFICATION_STATUS_ROLE, node.verificationStatusId);
    setOptionalRole(item, VERIFICATION_LEVEL_ROLE, node.verificationLevelId);
}

static void refreshDomainNodeText(QStandardItem *item) {
    QString text = item->data(NODE_NAME_ROLE).toString();
    const QVariant cognitiveLevel = item->data(COGNITIVE_LEVEL_ROLE);
    item->setToolTip({});
    if (cognitiveLevel.isValid()) {
        text += "  C" + QString::number(cognitiveLevel.toInt());
        for (const MetadataItem &level : AppState::metadata.cognitive_levels) {
            if (level.id == cognitiveLevel.toInt()) {
                item->setToolTip("Kognitives Niveau: " + metadataName(level));
                break;
            }
        }
    }
    const int flashcardCount = item->data(FLASHCARD_COUNT_ROLE).toInt();
    if (flashcardCount > 0) {
        text += QString::fromUtf8("  ▣ ") + QString::number(flashcardCount);
    }
    const int skillCount = item->data(SKILL_COUNT_ROLE).toInt();
    if (skillCount > 0) {
        text += QString::fromUtf8("  ★ ") + QString::number(skillCount);
    }
    item->setText(text);
}

static bool showDomainNodeDialog(
    QWidget *parent,
    QString &name,
    std::optional<int> &cognitiveLevelId,
    const bool hasDocument,
    std::optional<int> &completenessLevelId,
    std::optional<int> &flashcardStatusId,
    std::optional<int> &verificationStatusId,
    std::optional<int> &verificationLevelId
) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Domain Node bearbeiten");
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    auto *nameEdit = new QLineEdit(name, &dialog);
    auto *cognitiveCombo = new QComboBox(&dialog);
    fillMetadataCombo(cognitiveCombo, AppState::metadata.cognitive_levels);
    selectMetadata(cognitiveCombo, cognitiveLevelId);
    form->addRow("Name", nameEdit);
    form->addRow("Kognitives Niveau", cognitiveCombo);
    QComboBox *completenessCombo = nullptr;
    QComboBox *flashcardStatusCombo = nullptr;
    QComboBox *verificationStatusCombo = nullptr;
    QComboBox *verificationLevelCombo = nullptr;
    if (hasDocument) {
        completenessCombo = new QComboBox(&dialog);
        flashcardStatusCombo = new QComboBox(&dialog);
        verificationStatusCombo = new QComboBox(&dialog);
        verificationLevelCombo = new QComboBox(&dialog);
        fillMetadataCombo(
            completenessCombo,
            AppState::metadata.document_completeness_levels
        );
        fillMetadataCombo(
            flashcardStatusCombo,
            AppState::metadata.document_flashcard_statuses
        );
        fillMetadataCombo(
            verificationStatusCombo,
            AppState::metadata.document_verification_statuses
        );
        fillMetadataCombo(
            verificationLevelCombo,
            AppState::metadata.document_verification_levels
        );
        selectMetadata(completenessCombo, completenessLevelId);
        selectMetadata(flashcardStatusCombo, flashcardStatusId);
        selectMetadata(verificationStatusCombo, verificationStatusId);
        selectMetadata(verificationLevelCombo, verificationLevelId);
        form->addRow("Vollständigkeit", completenessCombo);
        form->addRow("Lernkartenstatus", flashcardStatusCombo);
        form->addRow("Verifizierungsstatus", verificationStatusCombo);
        form->addRow("Verifizierungsniveau", verificationLevelCombo);
    }
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return false;
    }

    name = nameEdit->text().trimmed();
    cognitiveLevelId = selectedMetadata(cognitiveCombo);
    if (hasDocument) {
        completenessLevelId = selectedMetadata(completenessCombo);
        flashcardStatusId = selectedMetadata(flashcardStatusCombo);
        verificationStatusId = selectedMetadata(verificationStatusCombo);
        verificationLevelId = selectedMetadata(verificationLevelCombo);
    }
    return true;
}

static bool showFlashcardDialog(
    QWidget *parent,
    const QString &title,
    QString &front,
    QString &back
) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.resize(480, 280);

    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    auto *frontEdit = new QPlainTextEdit(front, &dialog);
    auto *backEdit = new QPlainTextEdit(back, &dialog);
    form->addRow("Front", frontEdit);
    form->addRow("Back", backEdit);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        &dialog
    );
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    front = frontEdit->toPlainText().trimmed();
    back = backEdit->toPlainText().trimmed();
    return !front.isEmpty() && !back.isEmpty();
}

static bool showSkillDialog(
    QWidget *parent,
    const QString &title,
    QString &name,
    std::optional<int> &psychomotorLevelId
) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    auto *nameEdit = new QLineEdit(name, &dialog);
    auto *levelCombo = new QComboBox(&dialog);
    fillMetadataCombo(levelCombo, AppState::metadata.psychomotor_levels);
    selectMetadata(levelCombo, psychomotorLevelId);
    form->addRow("Name", nameEdit);
    form->addRow("Psychomotorisches Niveau", levelCombo);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return false;
    }

    name = nameEdit->text().trimmed();
    psychomotorLevelId = selectedMetadata(levelCombo);
    return true;
}

class LineNumberArea;

class SaveStatus : public QWidget {
public:
    enum State { Hidden, Saving, Saved, Error };

    explicit SaveStatus(QWidget *parent = nullptr) : QWidget(parent) {
        setFixedSize(24, 24);
        connect(&animationTimer, &QTimer::timeout, this, [this] {
            angle = (angle + 30) % 360;
            update();
        });
    }

    void setState(const State newState) {
        state = newState;
        if (state == Saving) {
            animationTimer.start(80);
            setToolTip("Wird gespeichert");
        } else {
            animationTimer.stop();
            setToolTip(state == Saved ? "Gespeichert" : state == Error ? "Speichern fehlgeschlagen" : "");
        }
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        if (state == Saving) {
            QPen pen(QColor(70, 145, 230), 2);
            pen.setCapStyle(Qt::RoundCap);
            painter.setPen(pen);
            painter.drawArc(rect().adjusted(4, 4, -4, -4), (90 - angle) * 16, 270 * 16);
        } else if (state == Saved) {
            painter.setPen(QPen(QColor(90, 140, 195), 1.5));
            painter.drawLine(5, 12, 10, 17);
            painter.drawLine(10, 17, 19, 7);
        } else if (state == Error) {
            painter.setPen(QColor(220, 80, 60));
            painter.setFont(QFont(painter.font().family(), 16));
            painter.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("\xE2\x9A\xA0"));
        }
    }

private:
    State state = Hidden;
    QTimer animationTimer;
    int angle = 0;
};

class DocumentEditor : public QPlainTextEdit {
public:
    explicit DocumentEditor(QWidget *parent = nullptr);
    int lineNumberAreaWidth() const;
    void paintLineNumbers(QPaintEvent *event);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);

    QWidget *lineNumberArea;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(DocumentEditor *editor)
        : QWidget(editor), editor(editor) {}

    QSize sizeHint() const override {
        return {editor->lineNumberAreaWidth(), 0};
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        editor->paintLineNumbers(event);
    }

private:
    DocumentEditor *editor;
};

DocumentEditor::DocumentEditor(QWidget *parent)
    : QPlainTextEdit(parent), lineNumberArea(new LineNumberArea(this)) {
    connect(this, &QPlainTextEdit::blockCountChanged, this,
            [this] { updateLineNumberAreaWidth(); });
    connect(this, &QPlainTextEdit::updateRequest, this,
            [this](const QRect &rect, const int dy) {
                updateLineNumberArea(rect, dy);
            });
    updateLineNumberAreaWidth();
}

int DocumentEditor::lineNumberAreaWidth() const {
    int digits = 1;
    for (int count = qMax(1, blockCount()); count >= 10; count /= 10) {
        ++digits;
    }

    return 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void DocumentEditor::updateLineNumberAreaWidth() {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void DocumentEditor::updateLineNumberArea(const QRect &rect, const int dy) {
    if (dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth();
    }
}

void DocumentEditor::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);
    const QRect contents = contentsRect();
    lineNumberArea->setGeometry(
        QRect(contents.left(), contents.top(), lineNumberAreaWidth(), contents.height())
    );
}

void DocumentEditor::paintLineNumbers(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), palette().color(QPalette::Base));
    painter.setPen(palette().color(QPalette::Light));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.drawText(
                0,
                top,
                lineNumberArea->width() - 4,
                fontMetrics().height(),
                Qt::AlignRight,
                QString::number(blockNumber + 1)
            );
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

DomainView::DomainView(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet("QSplitter::handle { background: palette(mid); }");

    treeView = new QTreeView(splitter);
    treeView->setHeaderHidden(true);
    treeView->setFrameShape(QFrame::NoFrame);
    treeView->setIndentation(16);
    treeView->setMinimumWidth(200);
    treeView->setMaximumWidth(600);
    treeView->setUniformRowHeights(true);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    model = new QStandardItemModel(treeView);
    treeView->setModel(model);

    editorWidget = new QWidget(splitter);
    editorWidget->setObjectName("documentEditorPanel");
    editorWidget->setStyleSheet(R"(
        QWidget#documentEditorPanel {
            background: palette(base);
        }
        QPlainTextEdit#documentEditor {
            background: palette(base);
            border: none;
        }
    )");
    editorLayout = new QVBoxLayout(editorWidget);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);

    documentWidget = new QWidget(editorWidget);
    auto *documentLayout = new QVBoxLayout(documentWidget);
    documentLayout->setContentsMargins(0, 0, 0, 0);
    documentLayout->setSpacing(0);

    auto *header = new QWidget(documentWidget);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(10, 6, 10, 6);
    documentTitle = new QLabel(documentWidget);
    QFont titleFont = documentTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    documentTitle->setFont(titleFont);
    documentEditor = new DocumentEditor(documentWidget);
    documentEditor->setObjectName("documentEditor");
    saveStatus = new SaveStatus(documentWidget);
    autosaveTimer = new QTimer(this);
    autosaveTimer->setInterval(1000);
    autosaveTimer->setSingleShot(true);
    headerLayout->addWidget(documentTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(saveStatus);
    documentLayout->addWidget(header);
    documentLayout->addWidget(documentEditor, 1);

    documentMetadataWidget = new QWidget(editorWidget);
    auto *documentMetadataLayout = new QHBoxLayout(documentMetadataWidget);
    documentMetadataLayout->setContentsMargins(10, 4, 10, 4);
    completenessLabel = new QLabel(documentMetadataWidget);
    flashcardStatusLabel = new QLabel(documentMetadataWidget);
    verificationStatusLabel = new QLabel(documentMetadataWidget);
    verificationLevelLabel = new QLabel(documentMetadataWidget);
    documentMetadataLayout->addStretch();
    documentMetadataLayout->addWidget(completenessLabel);
    documentMetadataLayout->addWidget(flashcardStatusLabel);
    documentMetadataLayout->addWidget(verificationStatusLabel);
    documentMetadataLayout->addWidget(verificationLevelLabel);

    flashcardWidget = new QWidget(editorWidget);
    auto *flashcardLayout = new QVBoxLayout(flashcardWidget);
    flashcardLayout->setContentsMargins(0, 0, 0, 0);
    flashcardLayout->setSpacing(0);

    auto *flashcardSeparator = new QFrame(flashcardWidget);
    flashcardSeparator->setFixedHeight(1);
    flashcardSeparator->setStyleSheet("background: palette(mid); border: none;");
    flashcardLayout->addWidget(flashcardSeparator);

    auto *flashcardHeader = new QWidget(flashcardWidget);
    flashcardHeader->setFixedHeight(36);
    auto *flashcardHeaderLayout = new QHBoxLayout(flashcardHeader);
    flashcardHeaderLayout->setContentsMargins(10, 4, 10, 4);
    flashcardTitle = new QLabel("Lernkarten (0)", flashcardHeader);
    auto *addFlashcardButton = new QPushButton("Lernkarte erstellen", flashcardHeader);
    addFlashcardButton->setToolTip("Lernkarte hinzufügen");
    toggleFlashcardsButton = new QPushButton("Lernkarten ausblenden", flashcardHeader);
    flashcardHeaderLayout->addWidget(flashcardTitle);
    flashcardHeaderLayout->addStretch();
    flashcardHeaderLayout->addWidget(addFlashcardButton);
    flashcardHeaderLayout->addWidget(toggleFlashcardsButton);
    flashcardLayout->addWidget(flashcardHeader);

    flashcardTable = new QTableView(flashcardWidget);
    flashcardTable->setFrameShape(QFrame::NoFrame);
    flashcardTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    flashcardTable->setSelectionMode(QAbstractItemView::SingleSelection);
    flashcardTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    flashcardTable->setMinimumHeight(0);
    flashcardTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    flashcardTable->setContextMenuPolicy(Qt::CustomContextMenu);
    flashcardModel = new QStandardItemModel(0, 2, flashcardTable);
    flashcardModel->setHorizontalHeaderLabels({"Front", "Back"});
    flashcardTable->setModel(flashcardModel);
    flashcardTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(flashcardTable->horizontalHeader(), &QHeaderView::sectionResized,
            this, [this] { updateFlashcardView(); });
    flashcardLayout->addWidget(flashcardTable, 1);

    editorLayout->addWidget(documentWidget);
    editorLayout->addStretch();
    editorLayout->addWidget(documentMetadataWidget);
    editorLayout->addWidget(flashcardWidget);
    editorWidget->hide();

    skillWidget = new QWidget(splitter);
    skillWidget->setObjectName("skillPanel");
    skillWidget->setMinimumWidth(210);
    skillWidget->setMaximumWidth(320);
    skillWidget->setStyleSheet(R"(
        QWidget#skillPanel {
            border-left: 1px solid palette(mid);
        }
    )");
    auto *skillLayout = new QVBoxLayout(skillWidget);
    skillLayout->setContentsMargins(8, 6, 4, 4);
    skillLayout->setSpacing(4);
    auto *skillHeaderLayout = new QHBoxLayout();
    skillHeaderLayout->setContentsMargins(0, 0, 0, 0);
    auto *skillTitle = new QLabel("Skillansicht", skillWidget);
    QFont skillTitleFont = skillTitle->font();
    skillTitleFont.setBold(true);
    skillTitle->setFont(skillTitleFont);
    addSkillButton = new QPushButton("+", skillWidget);
    addSkillButton->setFixedSize(24, 24);
    addSkillButton->setToolTip("Skill hinzufügen");
    skillHeaderLayout->addWidget(skillTitle);
    skillHeaderLayout->addStretch();
    skillHeaderLayout->addWidget(addSkillButton);
    skillLayout->addLayout(skillHeaderLayout);

    noSkillsLabel = new QLabel("Noch keine Skills angelegt.", skillWidget);
    noSkillsLabel->setWordWrap(true);
    noSkillsLabel->setStyleSheet("color: palette(mid);");
    skillLayout->addWidget(noSkillsLabel);

    skillTreeView = new QTreeView(skillWidget);
    skillTreeView->setHeaderHidden(true);
    skillTreeView->setFrameShape(QFrame::NoFrame);
    skillTreeView->setIndentation(14);
    skillTreeView->setUniformRowHeights(true);
    skillTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    skillTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    skillModel = new QStandardItemModel(skillTreeView);
    skillTreeView->setModel(skillModel);
    skillLayout->addWidget(skillTreeView, 1);
    skillWidget->hide();

    splitter->addWidget(treeView);
    splitter->addWidget(editorWidget);
    splitter->addWidget(skillWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    layout->addWidget(splitter);

    connect(&apiClient, &ApiClient::domainTreeReceived, this,
            [this](const std::vector<MetadataItem> &domains,
                   const std::vector<DomainNode> &nodes) {
                renderDomainTree(domains, nodes);
            });
    connect(&apiClient, &ApiClient::domainNodeCreated, this,
            [this](const DomainNode &node) {
                auto *item = new QStandardItem();
                item->setData(node.id, Qt::UserRole);
                item->setData(node.hasDocument, HAS_DOCUMENT_ROLE);
                setDomainNodeRoles(item, node);
                refreshDomainNodeText(item);
                QStandardItem *parentItem = node.parentId.has_value()
                    ? findDomainNodeItem(*node.parentId)
                    : findDomainItem(node.domainId);
                parentItem->appendRow(item);
            });
    connect(&apiClient, &ApiClient::domainNodeUpdated, this,
            [this](const DomainNode &node) {
                QStandardItem *item = findDomainNodeItem(node.id);
                setDomainNodeRoles(item, node);
                refreshDomainNodeText(item);
                if (currentDocumentNodeId == node.id) {
                    documentTitle->setText(node.name);
                    showMetadata(completenessLabel, "Vollständigkeit",
                                 AppState::metadata.document_completeness_levels,
                                 node.completenessLevelId);
                    showMetadata(flashcardStatusLabel, "Lernkarten",
                                 AppState::metadata.document_flashcard_statuses,
                                 node.flashcardStatusId);
                    showMetadata(verificationStatusLabel, "Verifizierung",
                                 AppState::metadata.document_verification_statuses,
                                 node.verificationStatusId);
                    showMetadata(verificationLevelLabel, "Niveau",
                                 AppState::metadata.document_verification_levels,
                                 node.verificationLevelId);
                }
            });
    connect(&apiClient, &ApiClient::documentReceived, this,
            [this](const Document &document) {
                if (!documentLoadPending || document.domainNodeId != pendingDocumentNodeId) {
                    return;
                }

                documentLoadPending = false;
                QStandardItem *item = findDomainNodeItem(document.domainNodeId);
                item->setData(true, HAS_DOCUMENT_ROLE);
                item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
                treeView->setCurrentIndex(item->index());
                currentDocumentNodeId = document.domainNodeId;
                documentTitle->setText(item->data(NODE_NAME_ROLE).toString());
                savedContent = document.content;
                documentEditor->setPlainText(document.content);
                showMetadata(completenessLabel, "Vollständigkeit",
                             AppState::metadata.document_completeness_levels,
                             document.completenessLevelId);
                showMetadata(flashcardStatusLabel, "Lernkarten",
                             AppState::metadata.document_flashcard_statuses,
                             document.flashcardStatusId);
                showMetadata(verificationStatusLabel, "Verifizierung",
                             AppState::metadata.document_verification_statuses,
                             document.verificationStatusId);
                showMetadata(verificationLevelLabel, "Niveau",
                             AppState::metadata.document_verification_levels,
                             document.verificationLevelId);
                static_cast<SaveStatus *>(saveStatus)->setState(SaveStatus::Hidden);
                editorWidget->show();
            });
    connect(&apiClient, &ApiClient::documentCreated, this,
            [this](const Document &document) {
                QStandardItem *item = findDomainNodeItem(document.domainNodeId);
                item->setData(true, HAS_DOCUMENT_ROLE);
                item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
                requestDocumentChange(document.domainNodeId, true);
            });
    connect(&apiClient, &ApiClient::documentUpdated, this,
            [this](const bool successful) {
                saveInProgress = false;
                if (!successful) {
                    static_cast<SaveStatus *>(saveStatus)->setState(SaveStatus::Error);
                    return;
                }

                savedContent = savingContent;
                static_cast<SaveStatus *>(saveStatus)->setState(SaveStatus::Saved);
                if (documentChangePending || closeContinuation || !autosaveTimer->isActive()) {
                    finishPendingActions();
                }
            });
    connect(treeView, &QTreeView::customContextMenuRequested, this,
            [this](const QPoint &position) {
                showDomainNodeContextMenu(position);
            });
    connect(treeView, &QTreeView::clicked, this, [this](const QModelIndex &index) {
        const QModelIndex nodeIndex = index.sibling(index.row(), 0);
        if (!nodeIndex.data(Qt::UserRole).isValid()) {
            requestDocumentChange(-1);
            return;
        }

        requestDocumentChange(
            nodeIndex.data(Qt::UserRole).toInt(),
            nodeIndex.data(HAS_DOCUMENT_ROLE).toBool()
        );
    });
    connect(addFlashcardButton, &QPushButton::clicked, this, &DomainView::createFlashcard);
    connect(toggleFlashcardsButton, &QPushButton::clicked, this, [this] {
        flashcardsExpanded = !flashcardsExpanded;
        updateFlashcardView();
    });
    connect(flashcardTable, &QTableView::doubleClicked, this,
            [this](const QModelIndex &index) { editFlashcard(index.row()); });
    connect(flashcardTable, &QTableView::customContextMenuRequested, this,
            [this](const QPoint &position) {
                const QModelIndex index = flashcardTable->indexAt(position);
                if (!index.isValid()) {
                    return;
                }

                QMenu menu(flashcardTable);
                QAction *deleteAction = menu.addAction("Löschen");
                if (menu.exec(flashcardTable->viewport()->mapToGlobal(position)) == deleteAction) {
                    apiClient.deleteFlashcard(
                        currentNodeId,
                        flashcardModel->item(index.row(), 0)->data(Qt::UserRole).toInt()
                    );
                }
            });
    connect(&apiClient, &ApiClient::nodeFlashcardsChanged, this,
            [this](const int nodeId) { apiClient.getNodeFlashcards(nodeId); });
    connect(&apiClient, &ApiClient::nodeFlashcardsReceived, this,
            [this](const int nodeId, const std::vector<Flashcard> &flashcards) {
                updateFlashcardCount(nodeId, static_cast<int>(flashcards.size()));
                if (nodeId != currentNodeId) {
                    return;
                }

                flashcardModel->removeRows(0, flashcardModel->rowCount());
                for (const Flashcard &flashcard : flashcards) {
                    auto *front = new QStandardItem(flashcard.front);
                    auto *back = new QStandardItem(flashcard.back);
                    front->setData(flashcard.id, Qt::UserRole);
                    flashcardModel->appendRow({front, back});
                }
                updateFlashcardView();
            });
    connect(addSkillButton, &QPushButton::clicked, this,
            [this] { createSkill(); });
    connect(skillTreeView, &QTreeView::customContextMenuRequested, this,
            [this](const QPoint &position) { showSkillContextMenu(position); });
    connect(skillTreeView, &QTreeView::doubleClicked, this,
            [this](const QModelIndex &index) { editSkill(index); });
    connect(&apiClient, &ApiClient::nodeSkillsChanged, this,
            [this](const int nodeId) { apiClient.getNodeSkills(nodeId); });
    connect(&apiClient, &ApiClient::nodeSkillsReceived, this,
            [this](const int nodeId, const std::vector<Skill> &skills) {
                updateSkillCount(nodeId, static_cast<int>(skills.size()));
                if (nodeId == currentNodeId) {
                    renderSkills(skills);
                }
            });
    connect(documentEditor, &QPlainTextEdit::textChanged, this, [this] {
        if (currentDocumentNodeId == -1) {
            return;
        }

        if (documentEditor->toPlainText() == savedContent) {
            autosaveTimer->stop();
            static_cast<SaveStatus *>(saveStatus)->setState(SaveStatus::Saved);
        } else {
            static_cast<SaveStatus *>(saveStatus)->setState(SaveStatus::Saving);
            autosaveTimer->start();
        }
    });
    connect(autosaveTimer, &QTimer::timeout, this, &DomainView::startSave);
    connect(&apiClient, &ApiClient::metadataReceived, this, [this] {
        apiClient.getDomainTree();
    });

    apiClient.getMetadata();
}

void DomainView::saveBeforeClose(std::function<void()> continuation) {
    closeContinuation = std::move(continuation);
    autosaveTimer->stop();
    finishPendingActions();
}

void DomainView::requestDocumentChange(const int nodeId, const bool hasDocument) {
    const bool currentHasDocument = currentDocumentNodeId != -1 || documentLoadPending;
    if (nodeId == currentNodeId && hasDocument == currentHasDocument
        && !documentChangePending) {
        return;
    }

    pendingDocumentNodeId = nodeId;
    pendingNodeHasDocument = hasDocument;
    documentChangePending = true;
    autosaveTimer->stop();
    finishPendingActions();
}

void DomainView::continueDocumentChange() {
    const int nodeId = pendingDocumentNodeId;
    const bool hasDocument = pendingNodeHasDocument;
    documentChangePending = false;
    currentNodeId = nodeId;
    currentDocumentNodeId = -1;
    savedContent.clear();
    documentLoadPending = nodeId != -1 && hasDocument;
    flashcardModel->removeRows(0, flashcardModel->rowCount());
    skillModel->removeRows(0, skillModel->rowCount());
    noSkillsLabel->show();
    skillTreeView->hide();
    editorWidget->setVisible(nodeId != -1);
    skillWidget->setVisible(nodeId != -1);
    documentWidget->setVisible(hasDocument);
    documentMetadataWidget->setVisible(hasDocument);
    editorLayout->setStretch(0, hasDocument ? 1 : 0);
    editorLayout->setStretch(1, hasDocument ? 0 : 1);
    updateFlashcardView();

    if (nodeId != -1) {
        apiClient.getNodeFlashcards(nodeId);
        apiClient.getNodeSkills(nodeId);
        if (hasDocument) {
            apiClient.getDocument(nodeId);
        }
    }
}

void DomainView::startSave() {
    if (saveInProgress || currentDocumentNodeId == -1
        || documentEditor->toPlainText() == savedContent) {
        return;
    }

    saveInProgress = true;
    savingContent = documentEditor->toPlainText();
    static_cast<SaveStatus *>(saveStatus)->setState(SaveStatus::Saving);
    apiClient.updateDocument(currentDocumentNodeId, savingContent);
}

void DomainView::finishPendingActions() {
    if (saveInProgress) {
        return;
    }

    if (currentDocumentNodeId != -1 && documentEditor->toPlainText() != savedContent) {
        startSave();
        return;
    }

    if (documentChangePending) {
        continueDocumentChange();
    }

    if (closeContinuation) {
        auto continuation = std::move(closeContinuation);
        closeContinuation = {};
        continuation();
    }
}

void DomainView::renderDomainTree(
    const std::vector<MetadataItem> &domains,
    const std::vector<DomainNode> &nodes
) {
    model->clear();
    model->setColumnCount(1);

    for (const MetadataItem &domain : domains) {
        auto *domainItem = new QStandardItem(domain.name);
        QFont font = domainItem->font();
        font.setBold(true);
        domainItem->setFont(font);
        domainItem->setData(domain.id, DOMAIN_ID_ROLE);
        model->appendRow(domainItem);

        std::function<void(std::optional<int>, QStandardItem *)> addNodes;
        addNodes = [&](const std::optional<int> parentId, QStandardItem *parentItem) {
            for (const DomainNode &node : nodes) {
                if (node.domainId != domain.id || node.parentId != parentId) {
                    continue;
                }

                auto *item = new QStandardItem();
                item->setData(node.id, Qt::UserRole);
                item->setData(node.hasDocument, HAS_DOCUMENT_ROLE);
                setDomainNodeRoles(item, node);
                refreshDomainNodeText(item);
                if (node.hasDocument) {
                    item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
                }
                parentItem->appendRow(item);
                addNodes(node.id, item);
            }
        };

        addNodes(std::nullopt, domainItem);
    }

    treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    treeView->collapseAll();
}

void DomainView::showDomainNodeContextMenu(const QPoint &position) {
    QModelIndex index = treeView->indexAt(position);

    if (!index.isValid()) {
        return;
    }

    index = index.sibling(index.row(), 0);

    if (index.data(DOMAIN_ID_ROLE).isValid()) {
        QMenu menu(treeView);
        QAction *createAction = menu.addAction("Root-Knoten hinzufügen");

        if (menu.exec(treeView->viewport()->mapToGlobal(position)) == createAction) {
            bool accepted;
            const QString name = QInputDialog::getText(
                treeView,
                "Root-Knoten hinzufügen",
                "Name",
                QLineEdit::Normal,
                {},
                &accepted
            ).trimmed();

            if (accepted && !name.isEmpty()) {
                apiClient.createRootDomainNode(index.data(DOMAIN_ID_ROLE).toInt(), name);
            }
        }

        return;
    }

    QMenu menu(treeView);
    QAction *createAction = menu.addAction("Unterknoten hinzufügen");
    QAction *editAction = menu.addAction("Bearbeiten");
    QAction *createDocumentAction = nullptr;
    if (!index.data(HAS_DOCUMENT_ROLE).toBool()) {
        createDocumentAction = menu.addAction("Dokument erstellen");
    }
    QAction *selectedAction = menu.exec(treeView->viewport()->mapToGlobal(position));

    if (selectedAction == createAction) {
        bool accepted;
        const QString name = QInputDialog::getText(
            treeView,
            "Unterknoten hinzufügen",
            "Name",
            QLineEdit::Normal,
            {},
            &accepted
        ).trimmed();

        if (accepted && !name.isEmpty()) {
            apiClient.createDomainNode(index.data(Qt::UserRole).toInt(), name);
        }
    } else if (selectedAction == editAction) {
        QString name = index.data(NODE_NAME_ROLE).toString();
        std::optional<int> cognitiveLevelId = optionalRole(index, COGNITIVE_LEVEL_ROLE);
        std::optional<int> completenessLevelId = optionalRole(
            index, COMPLETENESS_LEVEL_ROLE
        );
        std::optional<int> flashcardStatusId = optionalRole(index, FLASHCARD_STATUS_ROLE);
        std::optional<int> verificationStatusId = optionalRole(
            index, VERIFICATION_STATUS_ROLE
        );
        std::optional<int> verificationLevelId = optionalRole(
            index, VERIFICATION_LEVEL_ROLE
        );

        if (showDomainNodeDialog(
                treeView,
                name,
                cognitiveLevelId,
                index.data(HAS_DOCUMENT_ROLE).toBool(),
                completenessLevelId,
                flashcardStatusId,
                verificationStatusId,
                verificationLevelId
            )) {
            apiClient.updateDomainNode(
                index.data(Qt::UserRole).toInt(),
                name,
                cognitiveLevelId,
                completenessLevelId,
                flashcardStatusId,
                verificationStatusId,
                verificationLevelId
            );
        }
    } else if (createDocumentAction && selectedAction == createDocumentAction) {
        apiClient.createDocument(index.data(Qt::UserRole).toInt());
    }
}

void DomainView::createFlashcard() {
    if (currentNodeId == -1) {
        return;
    }

    QString front;
    QString back;
    if (showFlashcardDialog(this, "Lernkarte erstellen", front, back)) {
        apiClient.createFlashcard(currentNodeId, front, back);
    }
}

void DomainView::editFlashcard(const int row) {
    QStandardItem *frontItem = flashcardModel->item(row, 0);
    QStandardItem *backItem = flashcardModel->item(row, 1);
    QString front = frontItem->text();
    QString back = backItem->text();

    if (showFlashcardDialog(this, "Lernkarte bearbeiten", front, back)) {
        apiClient.updateFlashcard(
            currentNodeId,
            frontItem->data(Qt::UserRole).toInt(),
            front,
            back
        );
    }
}

void DomainView::renderSkills(const std::vector<Skill> &skills) {
    skillModel->clear();
    skillModel->setColumnCount(1);

    std::function<void(std::optional<int>, QStandardItem *)> addSkills;
    addSkills = [&](const std::optional<int> parentId, QStandardItem *parentItem) {
        for (const Skill &skill : skills) {
            if (skill.parentSkillId != parentId) {
                continue;
            }

            auto *item = new QStandardItem(skillText(skill));
            item->setData(skill.id, Qt::UserRole);
            item->setData(skill.name, SKILL_NAME_ROLE);
            setOptionalRole(item, PSYCHOMOTOR_LEVEL_ROLE, skill.psychomotorLevelId);
            if (parentItem) {
                parentItem->appendRow(item);
            } else {
                skillModel->appendRow(item);
            }
            addSkills(skill.id, item);
        }
    };
    addSkills(std::nullopt, nullptr);

    noSkillsLabel->setVisible(skills.empty());
    skillTreeView->setVisible(!skills.empty());
    skillTreeView->expandAll();
}

void DomainView::showSkillContextMenu(const QPoint &position) {
    const QModelIndex index = skillTreeView->indexAt(position);
    if (!index.isValid()) {
        return;
    }

    QMenu menu(skillTreeView);
    QAction *createAction = menu.addAction("Unter-Skill hinzufügen");
    QAction *editAction = menu.addAction("Bearbeiten");
    QAction *selectedAction = menu.exec(
        skillTreeView->viewport()->mapToGlobal(position)
    );

    if (selectedAction == createAction) {
        createSkill(index.data(Qt::UserRole).toInt());
    } else if (selectedAction == editAction) {
        editSkill(index);
    }
}

void DomainView::createSkill(const std::optional<int> parentSkillId) {
    if (currentNodeId == -1) {
        return;
    }

    QString name;
    std::optional<int> psychomotorLevelId;
    if (showSkillDialog(
            this,
            parentSkillId.has_value() ? "Unter-Skill hinzufügen" : "Skill hinzufügen",
            name,
            psychomotorLevelId
        )) {
        apiClient.createSkill(
            currentNodeId,
            parentSkillId,
            name,
            psychomotorLevelId
        );
    }
}

void DomainView::editSkill(const QModelIndex &index) {
    if (!index.isValid() || currentNodeId == -1) {
        return;
    }

    QString name = index.data(SKILL_NAME_ROLE).toString();
    std::optional<int> psychomotorLevelId = optionalRole(
        index,
        PSYCHOMOTOR_LEVEL_ROLE
    );
    if (showSkillDialog(this, "Skill bearbeiten", name, psychomotorLevelId)) {
        apiClient.updateSkill(
            currentNodeId,
            index.data(Qt::UserRole).toInt(),
            name,
            psychomotorLevelId
        );
    }
}

void DomainView::updateSkillCount(const int nodeId, const int count) {
    QStandardItem *item = findDomainNodeItem(nodeId);
    item->setData(count, SKILL_COUNT_ROLE);
    refreshDomainNodeText(item);
}

void DomainView::updateFlashcardCount(const int nodeId, const int count) {
    QStandardItem *item = findDomainNodeItem(nodeId);
    item->setData(count, FLASHCARD_COUNT_ROLE);
    refreshDomainNodeText(item);
}

void DomainView::updateFlashcardView() {
    const int count = flashcardModel->rowCount();
    flashcardTitle->setText("Lernkarten (" + QString::number(count) + ")");
    flashcardTitle->setVisible(count > 0);
    toggleFlashcardsButton->setVisible(count > 0);
    toggleFlashcardsButton->setText(
        flashcardsExpanded ? "Lernkarten ausblenden" : "Lernkarten anzeigen"
    );

    flashcardTable->setVisible(flashcardsExpanded && count > 0);
    int panelHeight = 37;

    if (flashcardsExpanded && count > 0) {
        flashcardTable->resizeRowsToContents();
        const int maximumRowHeight = flashcardTable->fontMetrics().lineSpacing() * 4 + 8;
        int height = flashcardTable->horizontalHeader()->height()
            + flashcardTable->frameWidth() * 2;
        for (int row = 0; row < count; ++row) {
            flashcardTable->setRowHeight(
                row,
                qMin(flashcardTable->rowHeight(row), maximumRowHeight)
            );
            height += flashcardTable->rowHeight(row);
        }

        const int maximumPanelHeight = qMax(37, editorWidget->height() / 2);
        panelHeight = qMin(37 + height, maximumPanelHeight);
        flashcardTable->setFixedHeight(panelHeight - 37);
    }

    flashcardWidget->setFixedHeight(panelHeight);
}

void DomainView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateFlashcardView();
}

QStandardItem *DomainView::findDomainItem(const int domainId) {
    const QModelIndexList matches = model->match(
        model->index(0, 0),
        DOMAIN_ID_ROLE,
        domainId,
        1,
        Qt::MatchExactly
    );

    return model->itemFromIndex(matches.first());
}

QStandardItem *DomainView::findDomainNodeItem(const int nodeId) {
    const QModelIndexList matches = model->match(
        model->index(0, 0),
        Qt::UserRole,
        nodeId,
        1,
        Qt::MatchExactly | Qt::MatchRecursive
    );

    return model->itemFromIndex(matches.first());
}
