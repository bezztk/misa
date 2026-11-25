#include "ShortTermLearningView.h"

#include "../api/ApiClient.h"
#include "DialogUtils.h"

#include <QCheckBox>
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
#include <QTimer>
#include <QVBoxLayout>

#include <limits>

namespace {

using State = ShortTermLearningState;
using Rating = ShortTermLearningRating;

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

void ShortTermLearningView::setBusy(const bool pending) {
    busy = pending;
    overview->setEnabled(!busy);
    details->setEnabled(!busy);
    updateSessionActions();
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
        cards = std::move(loaded->cards);
        ratingOptions = std::move(loaded->ratings);
        renderCards();
        if (screens->currentWidget() == session) {
            advanceSession(true);
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
            card.front, card.hint, card.back, stateName(card.frontToBack.state),
            card.backToFront ? stateName(card.backToFront->state) : QString{},
        });
        if (card.frontToBack.state != State::Completed ||
            (card.backToFront && card.backToFront->state != State::Completed)) {
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
    renderCards();
}

void ShortTermLearningView::advanceSession(const bool starting) {
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
            if (screens->currentWidget() == session) {
                advanceSession();
            } else {
                renderCards();
            }
            setBusy(false);
        });
}
