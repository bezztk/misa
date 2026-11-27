#include "ShortTermLearningView.h"

#include "ContextMenuStyle.h"

#include "../api/ApiClient.h"
#include "DialogUtils.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPalette>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStyle>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>

namespace {

using State = ShortTermLearningState;
using Rating = ShortTermLearningRating;

constexpr QSize setCardSize(320, 152);
constexpr int setCardSpacing = 12;
constexpr int setGridMargin = 16;
constexpr int forwardStateColumn = 3;
constexpr int reverseStateColumn = 4;

QString stateName(const State state) {
    switch (state) {
        case State::New: return "Neu";
        case State::Learning: return "Im Lernen";
        case State::Completed: return "Abgeschlossen";
    }
    return {};
}

QTableWidget *makeTable(const QStringList &columns, QWidget *parent) {
    auto *table = new QTableWidget(0, static_cast<int>(columns.size()), parent);
    table->setHorizontalHeaderLabels(columns);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->setStyleSheet(R"(
        QTableWidget {
            outline: 0;
            selection-background-color: #373737;
            selection-color: #EAEAEA;
        }
        QTableWidget::item:selected {
            background: #373737;
            color: #EAEAEA;
            border: none;
            border-radius: 0;
        }
    )");
    table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    return table;
}

void setRow(QTableWidget &table, const int row, const QStringList &values) {
    for (int column = 0; column < values.size(); ++column) {
        table.setItem(row, column, new QTableWidgetItem(values[column]));
    }
}

QLabel *plainLabel(QWidget *parent) {
    auto *label = new QLabel(parent);
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);
    return label;
}

void retainHiddenSpace(QWidget &widget) {
    auto policy = widget.sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    widget.setSizePolicy(policy);
}

QPushButton *addDialogButtons(QDialog &dialog, QVBoxLayout &layout, const QString &acceptText) {
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    auto *accept = buttons->button(QDialogButtonBox::Ok);
    accept->setText(acceptText);
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout.addWidget(buttons);
    return accept;
}

QWidget *makeSetCard(const ShortTermLearningSet &set, QWidget *parent) {
    auto *card = new QFrame(parent);
    card->setObjectName("shortTermSetCard");
    card->setFixedSize(setCardSize);
    card->setAttribute(Qt::WA_StyledBackground);
    card->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(4);

    auto *title = plainLabel(card);
    title->setWordWrap(false);
    auto font = title->font();
    font.setPixelSize(17);
    font.setBold(true);
    title->setFont(font);
    title->setText(QFontMetrics(font).elidedText(set.title, Qt::ElideRight,
        setCardSize.width() - layout->contentsMargins().left() - layout->contentsMargins().right()));
    layout->addWidget(title);

    auto *description = new QLabel(card);
    description->setText(QString("%1 · %2")
        .arg(set.cardCount == 1 ? "1 Karte" : QString("%1 Karten").arg(set.cardCount))
        .arg(set.bidirectional ? "Beide Richtungen" : "Vorne → Hinten"));
    layout->addWidget(description);

    auto *counts = new QGridLayout;
    counts->setHorizontalSpacing(4);
    const QStringList labels{"Neu", "Im Lernen", "Abgeschlossen"};
    const std::array values{set.newCount, set.learningCount, set.completedCount};
    for (int index = 0; index < labels.size(); ++index) {
        auto *count = new QLabel(QString("%1\n%2").arg(values[index]).arg(labels[index]), card);
        count->setAlignment(Qt::AlignCenter);
        counts->setColumnStretch(index, 1);
        counts->addWidget(count, 0, index);
    }
    layout->addLayout(counts, 1);

    const qint64 total = set.newCount + set.learningCount + set.completedCount;
    const int percentage = total > 0 ? static_cast<int>(set.completedCount * 100 / total) : 0;
    auto *progressRow = new QHBoxLayout;
    progressRow->setSpacing(8);
    auto *progress = new QProgressBar(card);
    progress->setObjectName("shortTermSetProgress");
    progress->setAccessibleName("Lernfortschritt");
    progress->setRange(0, 100);
    progress->setValue(percentage);
    progress->setTextVisible(false);
    progress->setFixedHeight(4);
    progressRow->addWidget(progress, 1);
    progressRow->addWidget(new QLabel(QString("%1 %").arg(percentage), card));
    layout->addLayout(progressRow);
    return card;
}

bool editCardText(QWidget *parent, ShortTermLearningCard &card, const bool creating) {
    QDialog dialog(parent);
    dialog.setWindowTitle(creating ? "Lernkarte erstellen" : "Lernkarte bearbeiten");
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto *front = new QPlainTextEdit(card.front, &dialog);
    auto *cardHint = new QPlainTextEdit(card.hint, &dialog);
    auto *back = new QPlainTextEdit(card.back, &dialog);
    for (auto *edit : {front, cardHint, back}) {
        edit->setFixedHeight(72);
    }
    form->addRow("Vorderseite", front);
    form->addRow("Hinweis", cardHint);
    form->addRow("Rückseite", back);
    layout->addLayout(form);
    auto *accept = addDialogButtons(dialog, *layout, creating ? "Erstellen" : "Speichern");
    const auto updateAccept = [accept, front, back] {
        accept->setEnabled(!front->toPlainText().trimmed().isEmpty() &&
                           !back->toPlainText().trimmed().isEmpty());
    };
    QObject::connect(front, &QPlainTextEdit::textChanged, &dialog, updateAccept);
    QObject::connect(back, &QPlainTextEdit::textChanged, &dialog, updateAccept);
    updateAccept();
    prepareDialog(dialog);
    if (execDialog(dialog) != QDialog::Accepted) {
        return false;
    }
    card.front = front->toPlainText();
    card.hint = cardHint->toPlainText();
    card.back = back->toPlainText();
    return true;
}

} // namespace

ShortTermLearningView::ShortTermLearningView(ApiClient &client, QWidget *parent)
    : QWidget(parent), apiClient(client) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    screens = new QStackedWidget(this);
    layout->addWidget(screens);
    overview = createOverview();
    details = createDetails();
    session = createSession();
    screens->addWidget(overview);
    screens->addWidget(details);
    screens->addWidget(session);
}

QWidget *ShortTermLearningView::createOverview() {
    auto *page = new QWidget(screens);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(setGridMargin, 12, setGridMargin, 0);
    setsGrid = new QListWidget(page);
    setsGrid->setObjectName("shortTermSetsGrid");
    setsGrid->setAccessibleName("Lernsätze");
    setsGrid->setFrameShape(QFrame::NoFrame);
    setsGrid->setViewMode(QListView::IconMode);
    setsGrid->setFlow(QListView::LeftToRight);
    setsGrid->setWrapping(true);
    setsGrid->setResizeMode(QListView::Adjust);
    setsGrid->setMovement(QListView::Static);
    setsGrid->setGridSize(setCardSize + QSize(setCardSpacing, setCardSpacing));
    setsGrid->setMinimumWidth(setCardSize.width()
        + style()->pixelMetric(QStyle::PM_ScrollBarExtent));
    setsGrid->setUniformItemSizes(true);
    setsGrid->setSelectionMode(QAbstractItemView::NoSelection);
    setsGrid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    setsGrid->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setsGrid->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setsGrid->setStyleSheet(R"(
        QListWidget#shortTermSetsGrid { background: palette(window); }
        QFrame#shortTermSetCard {
            background: transparent;
            border: 1px solid #424242;
            border-radius: 0;
        }
        QProgressBar#shortTermSetProgress {
            background: #353535;
            border: none;
            border-radius: 0;
        }
        QProgressBar#shortTermSetProgress::chunk {
            background: #737373;
            border-radius: 0;
        }
    )");
    auto *gridRow = new QHBoxLayout;
    gridRow->addStretch();
    gridRow->addWidget(setsGrid, 1);
    gridRow->addStretch();
    layout->addLayout(gridRow, 1);

    connect(setsGrid->verticalScrollBar(), &QScrollBar::rangeChanged, this, [this] {
        centerSetGrid();
    });
    connect(setsGrid, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        openSet(sets[setsGrid->row(item)]);
    });
    setsGrid->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(setsGrid, &QListWidget::customContextMenuRequested, this, [this](const QPoint &position) {
        if (auto *item = setsGrid->itemAt(position)) {
            showSetMenu(sets[setsGrid->row(item)].id, setsGrid->viewport()->mapToGlobal(position));
        }
    });
    return page;
}

QWidget *ShortTermLearningView::createDetails() {
    auto *page = new QWidget(screens);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(10);
    auto *header = new QHBoxLayout;
    header->setSpacing(10);
    auto *back = new QToolButton(page);
    back->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    back->setToolTip("Zur Übersicht");
    back->setAccessibleName("Zur Übersicht");
    back->setAutoRaise(true);
    back->setFixedSize(32, 32);
    setTitle = plainLabel(page);
    setTitle->setStyleSheet("font-size: 20px; font-weight: 600;");
    setTitle->setContextMenuPolicy(Qt::CustomContextMenu);
    startButton = new QPushButton("Lernsession starten", page);
    startButton->setMinimumSize(178, 42);
    startButton->setStyleSheet(R"(
        QPushButton { font-size: 15px; }
        QPushButton:enabled {
            background: palette(highlight);
            color: palette(highlighted-text);
            border: 1px solid transparent;
            border-radius: 6px;
        }
        QPushButton:hover:enabled, QPushButton:focus:enabled {
            border-color: palette(highlighted-text);
        }
    )");
    startButton->setEnabled(false);
    header->addWidget(back);
    header->addWidget(setTitle, 1);
    header->addWidget(startButton);
    layout->addLayout(header);

    cardsTable = makeTable(
        {"Vorderseite", "Hinweis", "Rückseite", "Vorne → Hinten", "Hinten → Vorne"}, page
    );
    for (int column = 0; column < 3; ++column) {
        cardsTable->horizontalHeader()->setSectionResizeMode(column, QHeaderView::Stretch);
    }
    layout->addWidget(cardsTable);

    connect(back, &QToolButton::clicked, this, &ShortTermLearningView::showOverview);
    connect(setTitle, &QLabel::customContextMenuRequested, this, [this](const QPoint &position) {
        showSetMenu(selectedSet.id, setTitle->mapToGlobal(position));
    });
    connect(cardsTable, &QTableWidget::cellDoubleClicked, this, &ShortTermLearningView::editCard);
    cardsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(cardsTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &position) {
        auto *item = cardsTable->itemAt(position);
        if (!item) {
            return;
        }
        const int row = item->row();
        QMenu menu(this);
        auto *edit = menu.addAction("Karte bearbeiten");
        auto *remove = menu.addAction(contextMenuDeleteIcon(), "Karte löschen");
        const auto *selected = menu.exec(cardsTable->viewport()->mapToGlobal(position));
        if (selected == edit) {
            editCard(row);
        } else if (selected == remove) {
            deleteCard(row);
        }
    });
    connect(startButton, &QPushButton::clicked, this, &ShortTermLearningView::startSession);
    return page;
}

QWidget *ShortTermLearningView::createSession() {
    auto *page = new QWidget(screens);
    page->setFocusPolicy(Qt::ClickFocus);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(20);

    auto *header = new QGridLayout;
    header->setColumnStretch(0, 1);
    header->setColumnStretch(2, 1);
    header->setHorizontalSpacing(12);
    remaining = new QLabel(page);
    sessionProgress = new QProgressBar(page);
    sessionProgress->setAccessibleName("Sitzungsfortschritt");
    sessionProgress->setTextVisible(false);
    sessionProgress->setFixedHeight(4);
    sessionProgress->setMinimumWidth(48);
    sessionProgress->setMaximumWidth(144);
    sessionProgress->setStyleSheet(R"(
        QProgressBar {
            background: rgba(127, 127, 127, 50);
            border: none;
            border-radius: 2px;
        }
        QProgressBar::chunk {
            background: rgba(127, 127, 127, 160);
            border-radius: 2px;
        }
    )");
    auto *end = new QPushButton("Lernsession beenden", page);
    header->addWidget(remaining, 0, 0, Qt::AlignLeft);
    header->addWidget(sessionProgress, 0, 1, Qt::AlignVCenter);
    header->addWidget(end, 0, 2, Qt::AlignRight);
    layout->addLayout(header);

    sessionContent = new QStackedWidget(page);
    sessionContent->setMaximumWidth(760);
    cardScroll = new QScrollArea(sessionContent);
    cardScroll->setFrameShape(QFrame::NoFrame);
    cardScroll->setWidgetResizable(true);
    auto *card = new QWidget(cardScroll);
    auto *content = new QVBoxLayout(card);
    content->setContentsMargins(12, 8, 12, 8);
    auto *fields = new QGridLayout;
    fields->setVerticalSpacing(16);
    fields->setRowMinimumHeight(0, 88);
    fields->setRowMinimumHeight(1, 76);
    fields->setRowMinimumHeight(2, 112);
    question = plainLabel(card);
    question->setStyleSheet("font-size: 34px; font-weight: 600;");
    hint = plainLabel(card);
    const bool darkBackground = card->palette().color(QPalette::Window).lightness() < 128;
    hint->setStyleSheet(QString("font-size: 18px; color: %1;")
        .arg(darkBackground ? "#a7a7a7" : "#626262"));
    answer = plainLabel(card);
    answer->setStyleSheet("font-size: 26px;");
    for (auto *label : {question, hint, answer}) {
        label->setAlignment(Qt::AlignCenter);
    }
    revealHint = new QPushButton("Hinweis anzeigen (I)", card);
    revealAnswer = new QPushButton("Antwort anzeigen (Leertaste)", card);
    for (auto *button : {revealHint, revealAnswer}) {
        button->setMinimumSize(184, 48);
        retainHiddenSpace(*button);
    }
    retainHiddenSpace(*hint);
    retainHiddenSpace(*answer);
    fields->addWidget(question, 0, 0);
    fields->addWidget(hint, 1, 0);
    fields->addWidget(revealHint, 1, 0, Qt::AlignCenter);
    fields->addWidget(answer, 2, 0);
    fields->addWidget(revealAnswer, 2, 0, Qt::AlignCenter);
    content->addStretch(1);
    content->addLayout(fields);
    content->addStretch(2);
    cardScroll->setWidget(card);
    card->setAutoFillBackground(false);
    cardScroll->viewport()->setAutoFillBackground(false);
    sessionContent->addWidget(cardScroll);

    countdown = plainLabel(sessionContent);
    countdown->setAlignment(Qt::AlignCenter);
    countdown->setStyleSheet("font-size: 22px;");
    sessionContent->addWidget(countdown);
    countdownTimer = new QTimer(page);
    countdownTimer->setSingleShot(true);
    countdownTimer->setTimerType(Qt::PreciseTimer);
    connect(countdownTimer, &QTimer::timeout, this, [this] { advanceSession(); });

    auto *cardBand = new QHBoxLayout;
    cardBand->addStretch();
    cardBand->addWidget(sessionContent, 24);
    cardBand->addStretch();
    layout->addLayout(cardBand, 1);

    const auto makeShortcut = [page](const Qt::Key key, QPushButton *button) {
        auto *shortcut = new QShortcut(QKeySequence(key), page);
        shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        shortcut->setAutoRepeat(false);
        shortcut->setEnabled(false);
        connect(shortcut, &QShortcut::activated, button, &QPushButton::click);
        return shortcut;
    };
    hintShortcut = makeShortcut(Qt::Key_I, revealHint);
    answerShortcut = makeShortcut(Qt::Key_Space, revealAnswer);

    ratingActions = new QWidget(page);
    auto *ratings = new QHBoxLayout(ratingActions);
    ratings->setContentsMargins(0, 0, 0, 0);
    ratings->setSpacing(12);
    constexpr std::array keys{Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4};
    ratings->addStretch();
    for (std::size_t index = 0; index < ratingButtons.size(); ++index) {
        auto *button = new QPushButton(ratingActions);
        button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        button->setMaximumWidth(170);
        button->setFixedHeight(56);
        ratings->addWidget(button, 8);
        ratingButtons[index] = button;
        ratingShortcuts[index] = makeShortcut(keys[index], button);
        connect(button, &QPushButton::clicked, this, [this, index] {
            rateQuestion(ratingOptions[index].rating);
        });
    }
    ratings->addStretch();
    ratingActions->setFixedHeight(56);
    retainHiddenSpace(*ratingActions);
    ratingActions->hide();
    ratingActions->setEnabled(false);
    layout->addWidget(ratingActions);

    connect(end, &QPushButton::clicked, this, &ShortTermLearningView::endSession);
    connect(revealHint, &QPushButton::clicked, this, [this] {
        revealHint->hide();
        hint->show();
        updateSessionActions();
        session->setFocus(Qt::OtherFocusReason);
    });
    connect(revealAnswer, &QPushButton::clicked, this, [this] {
        revealAnswer->hide();
        answer->show();
        ratingActions->show();
        updateSessionActions();
        session->setFocus(Qt::OtherFocusReason);
    });
    return page;
}

void ShortTermLearningView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (screens->currentWidget() == overview && !busy) {
        loadSets();
    }
}

void ShortTermLearningView::hideEvent(QHideEvent *event) {
    if (!event->spontaneous() && screens->currentWidget() == session) {
        endSession();
    }
    QWidget::hideEvent(event);
}

void ShortTermLearningView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    centerSetGrid();
}

void ShortTermLearningView::setBusy(const bool pending) {
    busy = pending;
    overview->setEnabled(!busy);
    details->setEnabled(!busy);
    updateSessionActions();
    if (!busy && createSetPending) {
        createSetPending = false;
        QTimer::singleShot(0, this, [this] {
            if (isVisible()) {
                openCreateSetDialog();
            }
        });
    }
}

void ShortTermLearningView::updateSessionActions() {
    const bool questionVisible = screens->currentWidget() == session &&
        sessionContent->currentWidget() == cardScroll;
    hintShortcut->setEnabled(questionVisible && !revealHint->isHidden());
    answerShortcut->setEnabled(questionVisible && !revealAnswer->isHidden());
    const bool canRate = questionVisible && !busy && !ratingActions->isHidden();
    ratingActions->setEnabled(canRate);
    for (auto *shortcut : ratingShortcuts) {
        shortcut->setEnabled(canRate);
    }
}

void ShortTermLearningView::loadSets() {
    setBusy(true);
    apiClient.getShortTermLearningSets(*this, [this](auto loaded) {
        setBusy(false);
        if (!loaded) {
            return;
        }
        sets = std::move(*loaded);
        setsGrid->clear();
        for (const auto &set : sets) {
            auto *item = new QListWidgetItem(setsGrid);
            item->setSizeHint(setCardSize);
            item->setData(Qt::AccessibleTextRole, set.title);
            item->setToolTip(set.title);
            setsGrid->setItemWidget(item, makeSetCard(set, setsGrid->viewport()));
        }
        centerSetGrid();
    });
}

void ShortTermLearningView::centerSetGrid() {
    const int availableWidth = width() - 2 * setGridMargin;
    const int columnWidth = setsGrid->gridSize().width();
    const int scrollBarWidth = setsGrid->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    const int columnsThatFit = qMax(1, (availableWidth - scrollBarWidth) / columnWidth);
    const int columns = qBound(1, setsGrid->count(), columnsThatFit);
    const int scrollSpace = setsGrid->verticalScrollBar()->maximum() > 0 ? scrollBarWidth : 0;
    setsGrid->setMaximumWidth(columns * columnWidth + scrollSpace);
}

void ShortTermLearningView::loadCards() {
    setBusy(true);
    apiClient.getShortTermLearningCards(selectedSet.id, *this, [this](auto loaded) {
        setBusy(false);
        if (!loaded) {
            if (screens->currentWidget() == session) {
                endSession();
            }
            return;
        }
        cards = std::move(loaded->cards);
        ratingOptions = std::move(loaded->ratings);
        renderCards();
        advanceSession(true);
    });
}

void ShortTermLearningView::renderCards() {
    cardsTable->setRowCount(0);
    cardsTable->setRowCount(static_cast<int>(cards.size()));
    int row = 0;
    for (const auto &card : cards) {
        setRow(*cardsTable, row++, {
            card.front, card.hint, card.back, stateName(card.frontToBack.state),
            card.backToFront ? stateName(card.backToFront->state) : QString{},
        });
    }
    updateStartButton();
}

void ShortTermLearningView::updateStartButton() {
    startButton->setEnabled(std::ranges::any_of(cards, [](const auto &card) {
        return card.frontToBack.state != State::Completed ||
            (card.backToFront && card.backToFront->state != State::Completed);
    }));
}

void ShortTermLearningView::openCreateSetDialog() {
    if (busy) {
        createSetPending = true;
        return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle("Lernsatz erstellen");
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto *title = new QLineEdit(&dialog);
    auto *bidirectional = new QCheckBox("In beide Richtungen abfragen", &dialog);
    form->addRow("Titel", title);
    form->addRow(bidirectional);
    layout->addLayout(form);
    auto *accept = addDialogButtons(dialog, *layout, "Erstellen");
    accept->setEnabled(false);
    connect(title, &QLineEdit::textChanged, &dialog, [accept](const QString &text) {
        accept->setEnabled(!text.trimmed().isEmpty());
    });
    prepareDialog(dialog);
    if (execDialog(dialog) != QDialog::Accepted) {
        return;
    }
    if (screens->currentWidget() == session) {
        endSession();
    }
    screens->setCurrentWidget(overview);
    setBusy(true);
    apiClient.createShortTermLearningSet(title->text().trimmed(), bidirectional->isChecked(), *this,
        [this](const bool saved) { saved ? loadSets() : setBusy(false); });
}

void ShortTermLearningView::showOverview() {
    screens->setCurrentWidget(overview);
    loadSets();
}

void ShortTermLearningView::openSet(const ShortTermLearningSet &set) {
    selectedSet = set;
    setTitle->setText(selectedSet.title);
    cardsTable->setColumnHidden(reverseStateColumn, !selectedSet.bidirectional);
    cards.clear();
    renderCards();
    screens->setCurrentWidget(details);
    loadCards();
}

void ShortTermLearningView::showSetMenu(const int setId, const QPoint &globalPosition) {
    QMenu menu(this);
    auto *create = menu.addAction("Lernkarte erstellen");
    auto *import = menu.addAction("JSON importieren");
    menu.addSeparator();
    auto *reset = menu.addAction("Fortschritt zurücksetzen");
    auto *remove = menu.addAction(contextMenuDeleteIcon(), "Lernsatz löschen");
    const auto *selected = menu.exec(globalPosition);
    if (selected == create) {
        createCard(setId);
    } else if (selected == import) {
        importCards(setId);
    } else if (selected == reset) {
        resetSet(setId);
    } else if (selected == remove) {
        deleteSet(setId);
    }
}

void ShortTermLearningView::refreshAfterSetChange(const bool saved) {
    if (!saved) {
        setBusy(false);
    } else if (screens->currentWidget() == details) {
        loadCards();
    } else {
        loadSets();
    }
}

void ShortTermLearningView::resetSet(const int setId) {
    if (QMessageBox::question(this, "Fortschritt zurücksetzen",
            "Alle Abfragen dieses Lernsatzes auf „Neu“ setzen?") != QMessageBox::Yes) {
        return;
    }
    setBusy(true);
    apiClient.resetShortTermLearningSet(setId, *this,
        [this](const bool saved) { refreshAfterSetChange(saved); });
}

void ShortTermLearningView::deleteSet(const int setId) {
    if (QMessageBox::question(this, "Lernsatz löschen",
            "Diesen Lernsatz mit allen Karten löschen?") != QMessageBox::Yes) {
        return;
    }
    setBusy(true);
    apiClient.deleteShortTermLearningSet(setId, *this,
        [this](const bool saved) { saved ? showOverview() : setBusy(false); });
}

void ShortTermLearningView::importCards(const int setId) {
    const QString path = QFileDialog::getOpenFileName(this, "Lernkarten importieren", {}, "JSON (*.json)");
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "JSON-Import", file.errorString());
        return;
    }
    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isArray()) {
        QMessageBox::warning(this, "JSON-Import", "Die Datei muss eine gültige JSON-Liste enthalten.");
        return;
    }
    const auto entries = document.array();
    for (qsizetype index = 0; index < entries.size(); ++index) {
        const auto entry = entries.at(index).toObject();
        const auto hintValue = entry.value("hint");
        if (entry.value("front").toString().trimmed().isEmpty() ||
            entry.value("back").toString().trimmed().isEmpty() ||
            (!hintValue.isUndefined() && !hintValue.isNull() && !hintValue.isString())) {
            QMessageBox::warning(this, "JSON-Import", QString(
                "Eintrag %1 braucht Text in „front“ und „back“. „hint“ ist optional und muss Text oder null sein."
            ).arg(index + 1));
            return;
        }
    }
    saveCards(setId, entries);
}

void ShortTermLearningView::saveCards(const int setId, const QJsonArray &entries) {
    setBusy(true);
    apiClient.importShortTermLearningCards(setId, entries, *this,
        [this](const bool saved) { refreshAfterSetChange(saved); });
}

void ShortTermLearningView::createCard(const int setId) {
    ShortTermLearningCard card{};
    if (editCardText(this, card, true)) {
        saveCards(setId, QJsonArray{QJsonObject{
            {"front", card.front},
            {"hint", card.hint.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(card.hint)},
            {"back", card.back},
        }});
    }
}

void ShortTermLearningView::editCard(const int row) {
    auto card = cards[row];
    if (!editCardText(this, card, false)) {
        return;
    }
    setBusy(true);
    apiClient.updateShortTermLearningCard(card, *this,
        [this](const bool saved) { saved ? loadCards() : setBusy(false); });
}

void ShortTermLearningView::deleteCard(const int row) {
    const int id = cards[row].id;
    if (QMessageBox::question(this, "Lernkarte löschen",
            "Diese Lernkarte löschen?") != QMessageBox::Yes) {
        return;
    }
    setBusy(true);
    apiClient.deleteShortTermLearningCard(id, *this,
        [this](const bool saved) { saved ? loadCards() : setBusy(false); });
}

void ShortTermLearningView::startSession() {
    remaining->setText("Lernkarten werden geladen …");
    sessionProgress->setRange(0, 1);
    sessionProgress->setValue(0);
    countdown->clear();
    sessionContent->setCurrentWidget(countdown);
    ratingActions->hide();
    screens->setCurrentWidget(session);
    session->setFocus(Qt::OtherFocusReason);
    loadCards();
}

void ShortTermLearningView::endSession() {
    countdownTimer->stop();
    screens->setCurrentWidget(details);
    updateSessionActions();
}

void ShortTermLearningView::advanceSession(const bool starting) {
    if (screens->currentWidget() != session) {
        return;
    }
    std::optional<Question> nextNew;
    std::optional<Question> nextReview;
    qint64 earliestDueAt = std::numeric_limits<qint64>::max();
    int openCount = 0;

    const auto consider = [&](const Question entry, const ShortTermLearningProgress &progress) {
        if (progress.state == State::Completed) {
            return;
        }
        ++openCount;
        if (progress.state == State::New) {
            if (!nextNew) {
                nextNew = entry;
            }
            return;
        }
        // A learning direction saved before scheduling was introduced is already due.
        const qint64 dueAt = progress.dueAt.isValid() ? progress.dueAt.toMSecsSinceEpoch() : 0;
        if (dueAt < earliestDueAt) {
            earliestDueAt = dueAt;
            nextReview = entry;
        }
    };
    for (std::size_t index = 0; index < cards.size(); ++index) {
        const auto &card = cards[index];
        consider({index, false}, card.frontToBack);
        if (card.backToFront) {
            consider({index, true}, *card.backToFront);
        }
    }

    if (openCount == 0) {
        sessionProgress->setValue(sessionProgress->maximum());
        endSession();
        return;
    }
    if (starting) {
        sessionProgress->setMaximum(openCount);
    }
    sessionProgress->setValue(sessionProgress->maximum() - openCount);
    remaining->setText(openCount == 1 ? "1 Abfrage verbleibend"
        : QString("%1 Abfragen verbleibend").arg(openCount));

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (nextReview && earliestDueAt <= now) {
        showQuestion(*nextReview);
    } else if (nextNew) {
        showQuestion(*nextNew);
    } else {
        const qint64 milliseconds = earliestDueAt - now;
        const qint64 seconds = (milliseconds + 999) / 1000;
        countdown->setText(QString("Nächste Abfrage in %1:%2")
            .arg(seconds / 60).arg(seconds % 60, 2, 10, QChar('0')));
        sessionContent->setCurrentWidget(countdown);
        ratingActions->hide();
        updateSessionActions();
        countdownTimer->start(static_cast<int>((milliseconds - 1) % 1000 + 1));
    }
}

void ShortTermLearningView::showQuestion(const Question next) {
    currentQuestion = next;
    const auto [index, reverse] = next;
    const auto &card = cards[index];
    question->setText(reverse ? card.back : card.front);
    answer->setText(reverse ? card.front : card.back);
    hint->setText(card.hint);
    hint->hide();
    answer->hide();
    revealHint->setVisible(!card.hint.isEmpty());
    revealAnswer->show();
    const auto &progress = reverse ? *card.backToFront : card.frontToBack;
    for (std::size_t index = 0; index < ratingButtons.size(); ++index) {
        const auto &option = ratingOptions[index];
        const bool completes = !option.intervalMinutes ||
            (option.rating == Rating::Good && progress.lastRating == Rating::Good);
        const QString interval = completes ? "Fertig" : QString("%1 Min.").arg(*option.intervalMinutes);
        ratingButtons[index]->setText(QString("%1 (%2)\n%3").arg(option.displayName)
            .arg(index + 1).arg(interval));
    }
    ratingActions->hide();
    sessionContent->setCurrentWidget(cardScroll);
    updateSessionActions();
    cardScroll->verticalScrollBar()->setValue(0);
}

void ShortTermLearningView::rateQuestion(const Rating rating) {
    if (!ratingActions->isEnabled()) {
        return;
    }
    const Question current = currentQuestion;
    session->setFocus(Qt::OtherFocusReason);
    setBusy(true);
    apiClient.rateShortTermLearningCard(cards[current.cardIndex].id, current.reverse, rating, *this,
        [this, current](const auto saved) {
            if (!saved) {
                setBusy(false);
                return;
            }
            auto &card = cards[current.cardIndex];
            auto &progress = current.reverse ? *card.backToFront : card.frontToBack;
            progress = *saved;
            const int column = current.reverse ? reverseStateColumn : forwardStateColumn;
            cardsTable->item(static_cast<int>(current.cardIndex), column)->setText(stateName(progress.state));
            updateStartButton();
            advanceSession();
            setBusy(false);
        });
}
