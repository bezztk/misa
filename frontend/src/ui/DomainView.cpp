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
#include <QTreeView>
#include <QVBoxLayout>

#include <functional>

static constexpr int DOMAIN_ID_ROLE = Qt::UserRole + 1;
static constexpr int HAS_DOCUMENT_ROLE = Qt::UserRole + 2;

class LineNumberArea;

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
    saveButton = new QPushButton("Speichern", editorWidget);
    headerLayout->addWidget(documentTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(saveButton);
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
                QStandardItem *item = findDomainNodeItem(document.domainNodeId);
                item->setData(true, HAS_DOCUMENT_ROLE);
                item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
                treeView->setCurrentIndex(item->index());
                currentDocumentNodeId = document.domainNodeId;
                documentTitle->setText(item->text());
                documentEditor->setPlainText(document.content);
                editorWidget->show();
            });
    connect(treeView, &QTreeView::customContextMenuRequested, this,
            [this](const QPoint &position) {
                showDomainNodeContextMenu(position);
            });
    connect(treeView, &QTreeView::clicked, this, [this](const QModelIndex &index) {
        if (!index.data(Qt::UserRole).isValid()
            || !index.data(HAS_DOCUMENT_ROLE).toBool()) {
            currentDocumentNodeId = -1;
            editorWidget->hide();
            return;
        }

        editorWidget->hide();
        apiClient.getDocument(index.data(Qt::UserRole).toInt());
    });
    connect(saveButton, &QPushButton::clicked, this, [this] {
        apiClient.updateDocument(currentDocumentNodeId, documentEditor->toPlainText());
    });

    apiClient.getDomainTree();
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
