#include "ShortTermLearningView.h"

#include "../api/ApiClient.h"
#include "DialogUtils.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
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
#include <QMessageBox>
#include <QPalette>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QShowEvent>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

using State = ShortTermLearningState;

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

QComboBox *stateEditor(const State selected, QWidget *parent) {
    auto *editor = new QComboBox(parent);
    for (const State state : {State::New, State::Learning, State::Completed}) {
        editor->addItem(stateName(state), static_cast<int>(state));
    }
    editor->setCurrentIndex(editor->findData(static_cast<int>(selected)));
    return editor;
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
    auto *actions = new QHBoxLayout;
    auto *create = new QPushButton("Lernsatz erstellen", page);
    auto *open = new QPushButton("Öffnen", page);
    auto *reset = new QPushButton("Fortschritt zurücksetzen", page);
    auto *remove = new QPushButton("Lernsatz löschen", page);
    actions->addWidget(create);
    actions->addWidget(open);
    actions->addWidget(reset);
    actions->addWidget(remove);
    actions->addStretch();
    layout->addLayout(actions);

    setsTable = makeTable(
        {"Titel", "Karten", "Abfragerichtung", "Neu", "Im Lernen", "Abgeschlossen", "Fortschritt"},
        page
    );
    setsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    layout->addWidget(setsTable);

    const auto updateActions = [this, open, reset, remove] {
        const bool selected = setsTable->currentRow() >= 0;
        open->setEnabled(selected);
        reset->setEnabled(selected);
        remove->setEnabled(selected);
    };
    connect(setsTable, &QTableWidget::currentCellChanged, this, updateActions);
    updateActions();

    connect(create, &QPushButton::clicked, this, &ShortTermLearningView::createSet);
    connect(open, &QPushButton::clicked, this, &ShortTermLearningView::openSet);
    connect(setsTable, &QTableWidget::cellDoubleClicked, this, &ShortTermLearningView::openSet);
    connect(reset, &QPushButton::clicked, this, &ShortTermLearningView::resetSet);
    connect(remove, &QPushButton::clicked, this, &ShortTermLearningView::deleteSet);
    return page;
}

QWidget *ShortTermLearningView::createDetails() {
    auto *page = new QWidget(screens);
    auto *layout = new QVBoxLayout(page);
    setTitle = plainLabel(page);
    layout->addWidget(setTitle);

    auto *actions = new QHBoxLayout;
    auto *back = new QPushButton("Zur Übersicht", page);
    auto *import = new QPushButton("JSON importieren", page);
    auto *edit = new QPushButton("Karte bearbeiten", page);
    auto *remove = new QPushButton("Karte löschen", page);
    startButton = new QPushButton("Lernsession starten", page);
    startButton->setEnabled(false);
    actions->addWidget(back);
    actions->addWidget(import);
    actions->addWidget(edit);
    actions->addWidget(remove);
    actions->addStretch();
    actions->addWidget(startButton);
    layout->addLayout(actions);

    cardsTable = makeTable(
        {"Vorderseite", "Hinweis", "Rückseite", "Vorne → Hinten", "Hinten → Vorne"}, page
    );
    for (int column = 0; column < 3; ++column) {
        cardsTable->horizontalHeader()->setSectionResizeMode(column, QHeaderView::Stretch);
    }
    layout->addWidget(cardsTable);

    const auto updateActions = [this, edit, remove] {
        const bool selected = cardsTable->currentRow() >= 0;
        edit->setEnabled(selected);
        remove->setEnabled(selected);
    };
    connect(cardsTable, &QTableWidget::currentCellChanged, this, updateActions);
    updateActions();

    connect(back, &QPushButton::clicked, this, [this] {
        screens->setCurrentWidget(overview);
        loadSets();
    });
    connect(import, &QPushButton::clicked, this, &ShortTermLearningView::importCards);
    connect(edit, &QPushButton::clicked, this, &ShortTermLearningView::editCard);
    connect(cardsTable, &QTableWidget::cellDoubleClicked, this, &ShortTermLearningView::editCard);
    connect(remove, &QPushButton::clicked, this, &ShortTermLearningView::deleteCard);
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

    cardScroll = new QScrollArea(page);
    cardScroll->setFrameShape(QFrame::NoFrame);
    cardScroll->setWidgetResizable(true);
    cardScroll->setMaximumWidth(760);
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

    auto *cardBand = new QHBoxLayout;
    cardBand->addStretch();
    cardBand->addWidget(cardScroll, 24);
    cardBand->addStretch();
    layout->addLayout(cardBand, 1);

    ratingActions = new QWidget(page);
    auto *ratings = new QHBoxLayout(ratingActions);
    ratings->setContentsMargins(0, 0, 0, 0);
    ratings->setSpacing(12);
    auto *continueLearning = new QPushButton("Weiterlernen (1)", ratingActions);
    auto *complete = new QPushButton("Beherrscht (2)", ratingActions);
    ratings->addStretch();
    for (auto *button : {continueLearning, complete}) {
        button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        button->setMaximumWidth(170);
        button->setFixedHeight(56);
        ratings->addWidget(button, 8);
    }
    ratings->addStretch();
    ratingActions->setFixedHeight(56);
    retainHiddenSpace(*ratingActions);
    ratingActions->hide();
    ratingActions->setEnabled(false);
    layout->addWidget(ratingActions);

    hintShortcut = new QShortcut(QKeySequence(Qt::Key_I), page);
    answerShortcut = new QShortcut(QKeySequence(Qt::Key_Space), page);
    continueShortcut = new QShortcut(QKeySequence(Qt::Key_1), page);
    completeShortcut = new QShortcut(QKeySequence(Qt::Key_2), page);
    for (auto *shortcut : {hintShortcut, answerShortcut, continueShortcut, completeShortcut}) {
        shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        shortcut->setAutoRepeat(false);
        shortcut->setEnabled(false);
    }
    connect(hintShortcut, &QShortcut::activated, revealHint, &QPushButton::click);
    connect(answerShortcut, &QShortcut::activated, revealAnswer, &QPushButton::click);
    connect(continueShortcut, &QShortcut::activated, continueLearning, &QPushButton::click);
    connect(completeShortcut, &QShortcut::activated, complete, &QPushButton::click);
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
    connect(continueLearning, &QPushButton::clicked, this, [this] { rateQuestion(State::Learning); });
    connect(complete, &QPushButton::clicked, this, [this] { rateQuestion(State::Completed); });
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

void ShortTermLearningView::setBusy(const bool pending) {
    busy = pending;
    overview->setEnabled(!busy);
    details->setEnabled(!busy);
    updateSessionActions();
}

void ShortTermLearningView::updateSessionActions() {
    const bool questionVisible = screens->currentWidget() == session && !cardScroll->isHidden();
    hintShortcut->setEnabled(questionVisible && !revealHint->isHidden());
    answerShortcut->setEnabled(questionVisible && !revealAnswer->isHidden());
    const bool canRate = questionVisible && !busy && !ratingActions->isHidden();
    ratingActions->setEnabled(canRate);
    continueShortcut->setEnabled(canRate);
    completeShortcut->setEnabled(canRate);
}

void ShortTermLearningView::loadSets() {
    setBusy(true);
    apiClient.getShortTermLearningSets(*this, [this](auto loaded) {
        setBusy(false);
        if (!loaded) {
            return;
        }
        sets = std::move(*loaded);
        setsTable->setRowCount(0);
        setsTable->setRowCount(static_cast<int>(sets.size()));
        int row = 0;
        for (const auto &set : sets) {
            const qint64 total = set.newCount + set.learningCount + set.completedCount;
            const qint64 progress = total > 0 ? set.completedCount * 100 / total : 0;
            setRow(*setsTable, row++, {
                set.title,
                QString::number(set.cardCount),
                set.bidirectional ? "Beide Richtungen" : "Vorne → Hinten",
                QString::number(set.newCount),
                QString::number(set.learningCount),
                QString::number(set.completedCount),
                QString("%1 %").arg(progress),
            });
        }
    });
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
        cards = std::move(*loaded);
        renderCards();
        if (screens->currentWidget() != session) {
            return;
        }

        for (std::size_t index = 0; index < cards.size(); ++index) {
            if (cards[index].frontToBackState != State::Completed) {
                queue.push_back({index, false});
            }
        }
        for (std::size_t index = 0; index < cards.size(); ++index) {
            if (cards[index].backToFrontState && cards[index].backToFrontState != State::Completed) {
                queue.push_back({index, true});
            }
        }
        if (queue.empty()) {
            endSession();
        } else {
            sessionProgress->setMaximum(static_cast<int>(queue.size()));
            showQuestion();
        }
    });
}

void ShortTermLearningView::renderCards() {
    cardsTable->setRowCount(0);
    cardsTable->setRowCount(static_cast<int>(cards.size()));
    startButton->setEnabled(false);
    int row = 0;
    for (const auto &card : cards) {
        setRow(*cardsTable, row++, {
            card.front, card.hint, card.back, stateName(card.frontToBackState),
            card.backToFrontState ? stateName(*card.backToFrontState) : QString{},
        });
        if (card.frontToBackState != State::Completed ||
            (card.backToFrontState && card.backToFrontState != State::Completed)) {
            startButton->setEnabled(true);
        }
    }
}

void ShortTermLearningView::createSet() {
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
    setBusy(true);
    apiClient.createShortTermLearningSet(title->text().trimmed(), bidirectional->isChecked(), *this,
        [this](const bool saved) { saved ? loadSets() : setBusy(false); });
}

void ShortTermLearningView::openSet() {
    const int row = setsTable->currentRow();
    if (row < 0) {
        return;
    }
    selectedSet = sets[row];
    setTitle->setText(selectedSet.title);
    cardsTable->setColumnHidden(4, !selectedSet.bidirectional);
    cards.clear();
    renderCards();
    screens->setCurrentWidget(details);
    loadCards();
}

void ShortTermLearningView::resetSet() {
    const int id = sets[setsTable->currentRow()].id;
    if (QMessageBox::question(this, "Fortschritt zurücksetzen",
            "Alle Abfragen dieses Lernsatzes auf „Neu“ setzen?") != QMessageBox::Yes) {
        return;
    }
    setBusy(true);
    apiClient.resetShortTermLearningSet(id, *this,
        [this](const bool saved) { saved ? loadSets() : setBusy(false); });
}

void ShortTermLearningView::deleteSet() {
    const int id = sets[setsTable->currentRow()].id;
    if (QMessageBox::question(this, "Lernsatz löschen",
            "Diesen Lernsatz mit allen Karten löschen?") != QMessageBox::Yes) {
        return;
    }
    setBusy(true);
    apiClient.deleteShortTermLearningSet(id, *this,
        [this](const bool saved) { saved ? loadSets() : setBusy(false); });
}

void ShortTermLearningView::importCards() {
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
    setBusy(true);
    apiClient.importShortTermLearningCards(selectedSet.id, entries, *this,
        [this](const bool saved) { saved ? loadCards() : setBusy(false); });
}

void ShortTermLearningView::editCard() {
    const int row = cardsTable->currentRow();
    if (row < 0) {
        return;
    }
    auto card = cards[row];
    QDialog dialog(this);
    dialog.setWindowTitle("Lernkarte bearbeiten");
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
    auto *forwardState = stateEditor(card.frontToBackState, &dialog);
    form->addRow("Vorne → Hinten", forwardState);
    QComboBox *reverseState = nullptr;
    if (card.backToFrontState) {
        reverseState = stateEditor(*card.backToFrontState, &dialog);
        form->addRow("Hinten → Vorne", reverseState);
    }
    layout->addLayout(form);
    auto *accept = addDialogButtons(dialog, *layout, "Speichern");
    const auto updateAccept = [accept, front, back] {
        accept->setEnabled(!front->toPlainText().trimmed().isEmpty() &&
                           !back->toPlainText().trimmed().isEmpty());
    };
    connect(front, &QPlainTextEdit::textChanged, &dialog, updateAccept);
    connect(back, &QPlainTextEdit::textChanged, &dialog, updateAccept);
    updateAccept();
    prepareDialog(dialog);
    if (execDialog(dialog) != QDialog::Accepted) {
        return;
    }
    card.front = front->toPlainText();
    card.hint = cardHint->toPlainText();
    card.back = back->toPlainText();
    card.frontToBackState = static_cast<State>(forwardState->currentData().toInt());
    if (reverseState) {
        card.backToFrontState = static_cast<State>(reverseState->currentData().toInt());
    }
    setBusy(true);
    apiClient.updateShortTermLearningCard(card, *this,
        [this](const bool saved) { saved ? loadCards() : setBusy(false); });
}

void ShortTermLearningView::deleteCard() {
    const int id = cards[cardsTable->currentRow()].id;
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
    cardScroll->hide();
    ratingActions->hide();
    screens->setCurrentWidget(session);
    session->setFocus(Qt::OtherFocusReason);
    loadCards();
}

void ShortTermLearningView::endSession() {
    queue.clear();
    screens->setCurrentWidget(details);
    updateSessionActions();
    renderCards();
}

void ShortTermLearningView::showQuestion() {
    const auto [index, reverse] = queue.front();
    const auto &card = cards[index];
    question->setText(reverse ? card.back : card.front);
    answer->setText(reverse ? card.front : card.back);
    hint->setText(card.hint);
    hint->hide();
    answer->hide();
    revealHint->setVisible(!card.hint.isEmpty());
    revealAnswer->show();
    ratingActions->hide();
    remaining->setText(queue.size() == 1 ? "1 Abfrage verbleibend"
        : QString("%1 Abfragen verbleibend").arg(queue.size()));
    cardScroll->show();
    updateSessionActions();
    cardScroll->verticalScrollBar()->setValue(0);
}

void ShortTermLearningView::rateQuestion(const State state) {
    if (!ratingActions->isEnabled()) {
        return;
    }
    const Question current = queue.front();
    session->setFocus(Qt::OtherFocusReason);
    setBusy(true);
    apiClient.rateShortTermLearningCard(cards[current.cardIndex].id, current.reverse, state, *this,
        [this, current, state](const bool saved) {
            if (!saved) {
                setBusy(false);
                return;
            }
            auto &card = cards[current.cardIndex];
            if (current.reverse) {
                card.backToFrontState = state;
            } else {
                card.frontToBackState = state;
            }
            if (screens->currentWidget() != session) {
                renderCards();
                setBusy(false);
                return;
            }
            queue.pop_front();
            if (state == State::Learning) {
                queue.push_back(current);
            }
            sessionProgress->setValue(sessionProgress->maximum() - static_cast<int>(queue.size()));
            if (queue.empty()) {
                endSession();
            } else {
                showQuestion();
            }
            setBusy(false);
        });
}
