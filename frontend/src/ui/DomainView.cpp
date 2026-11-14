#include "DomainView.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStandardItemModel>
#include <QStyle>
#include <QTextBlock>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

#include <functional>

static constexpr int DOMAIN_ID_ROLE = Qt::UserRole + 1;
static constexpr int HAS_DOCUMENT_ROLE = Qt::UserRole + 2;

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
    auto *editorLayout = new QVBoxLayout(editorWidget);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);

    auto *header = new QWidget(editorWidget);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(10, 6, 10, 6);
    documentTitle = new QLabel(editorWidget);
    QFont titleFont = documentTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    documentTitle->setFont(titleFont);
    documentEditor = new DocumentEditor(editorWidget);
    documentEditor->setObjectName("documentEditor");
    saveStatus = new SaveStatus(editorWidget);
    autosaveTimer = new QTimer(this);
    autosaveTimer->setInterval(1000);
    autosaveTimer->setSingleShot(true);
    headerLayout->addWidget(documentTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(saveStatus);
    editorLayout->addWidget(header);
    editorLayout->addWidget(documentEditor, 1);
    editorWidget->hide();

    splitter->addWidget(treeView);
    splitter->addWidget(editorWidget);
    layout->addWidget(splitter);

    connect(&apiClient, &ApiClient::domainTreeReceived, this,
            [this](const std::vector<MetadataItem> &domains,
                   const std::vector<DomainNode> &nodes) {
                renderDomainTree(domains, nodes);
            });
    connect(&apiClient, &ApiClient::domainNodeCreated, this,
            [this](const DomainNode &node) {
                auto *item = new QStandardItem(node.name);
                item->setData(node.id, Qt::UserRole);
                item->setData(node.hasDocument, HAS_DOCUMENT_ROLE);
                QStandardItem *parentItem = node.parentId.has_value()
                    ? findDomainNodeItem(*node.parentId)
                    : findDomainItem(node.domainId);
                parentItem->appendRow(item);
            });
    connect(&apiClient, &ApiClient::domainNodeUpdated, this,
            [this](const DomainNode &node) {
                QStandardItem *item = findDomainNodeItem(node.id);
                item->setText(node.name);
                if (currentDocumentNodeId == node.id) {
                    documentTitle->setText(node.name);
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
                documentTitle->setText(item->text());
                savedContent = document.content;
                documentEditor->setPlainText(document.content);
                static_cast<SaveStatus *>(saveStatus)->setState(SaveStatus::Hidden);
                editorWidget->show();
            });
    connect(&apiClient, &ApiClient::documentCreated, this,
            [this](const Document &document) {
                QStandardItem *item = findDomainNodeItem(document.domainNodeId);
                item->setData(true, HAS_DOCUMENT_ROLE);
                item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
                requestDocumentChange(document.domainNodeId);
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
        if (!index.data(Qt::UserRole).isValid()
            || !index.data(HAS_DOCUMENT_ROLE).toBool()) {
            requestDocumentChange(-1);
            return;
        }

        requestDocumentChange(index.data(Qt::UserRole).toInt());
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

    apiClient.getDomainTree();
}

void DomainView::saveBeforeClose(std::function<void()> continuation) {
    closeContinuation = std::move(continuation);
    autosaveTimer->stop();
    finishPendingActions();
}

void DomainView::requestDocumentChange(const int nodeId) {
    if (nodeId == currentDocumentNodeId && !documentChangePending) {
        return;
    }

    pendingDocumentNodeId = nodeId;
    documentChangePending = true;
    autosaveTimer->stop();
    finishPendingActions();
}

void DomainView::continueDocumentChange() {
    const int nodeId = pendingDocumentNodeId;
    documentChangePending = false;
    currentDocumentNodeId = -1;
    editorWidget->hide();
    documentLoadPending = nodeId != -1;

    if (nodeId != -1) {
        apiClient.getDocument(nodeId);
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

                auto *item = new QStandardItem(node.name);
                item->setData(node.id, Qt::UserRole);
                item->setData(node.hasDocument, HAS_DOCUMENT_ROLE);
                if (node.hasDocument) {
                    item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
                }
                parentItem->appendRow(item);
                addNodes(node.id, item);
            }
        };

        addNodes(std::nullopt, domainItem);
    }

    treeView->collapseAll();
}

void DomainView::showDomainNodeContextMenu(const QPoint &position) {
    const QModelIndex index = treeView->indexAt(position);

    if (!index.isValid()) {
        return;
    }

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
        bool accepted;
        const QString name = QInputDialog::getText(
            treeView,
            "Domain Node bearbeiten",
            "Name",
            QLineEdit::Normal,
            index.data().toString(),
            &accepted
        ).trimmed();

        if (accepted && !name.isEmpty()) {
            apiClient.updateDomainNode(index.data(Qt::UserRole).toInt(), name);
        }
    } else if (createDocumentAction && selectedAction == createDocumentAction) {
        apiClient.createDocument(index.data(Qt::UserRole).toInt());
    }
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
