#include "PasswordManagerView.h"

#include "../api/ApiClient.h"
#include "DialogUtils.h"

#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include <sodium.h>

#include <algorithm>

PasswordManagerView::PasswordManagerView(ApiClient &apiClient, QWidget *parent)
    : QWidget(parent), apiClient(apiClient) {
    if (sodium_init() < 0) {
        qFatal("libsodium could not be initialized");
    }

    setStyleSheet("PasswordManagerView { background: #1C1C1C; }");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    pages = new QStackedWidget(this);
    layout->addWidget(pages);

    lockedPage = new QWidget(pages);
    auto *lockedLayout = new QVBoxLayout(lockedPage);
    lockedLayout->addStretch();
    auto *lockedLabel = new QLabel("Der Passwortmanager ist gesperrt.", lockedPage);
    lockedLabel->setAlignment(Qt::AlignCenter);
    lockedLayout->addWidget(lockedLabel);
    auto *unlockButton = new QPushButton("Mit Masterpasswort entsperren", lockedPage);
    lockedLayout->addWidget(unlockButton, 0, Qt::AlignCenter);
    lockedLayout->addStretch();
    pages->addWidget(lockedPage);

    auto *scrollArea = new QScrollArea(pages);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: #1C1C1C; }");
    scrollArea->viewport()->setStyleSheet("background: #1C1C1C;");
    entriesPage = scrollArea;
    auto *cardContainer = new QWidget(scrollArea);
    cardContainer->setStyleSheet("background: #1C1C1C;");
    cardLayout = new QVBoxLayout(cardContainer);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(0);
    scrollArea->setWidget(cardContainer);
    pages->addWidget(entriesPage);

    connect(unlockButton, &QPushButton::clicked, this, &PasswordManagerView::unlock);

    connect(&apiClient, &ApiClient::passwordEntriesReceived,
            this, &PasswordManagerView::receiveEntries);
    connect(&apiClient, &ApiClient::passwordCategoriesReceived, this,
            [this](const std::vector<PasswordCategory> &received) {
                categories = received;
                categoriesLoaded = true;
                renderEntries();
                runPendingAction();
            });
    connect(&apiClient, &ApiClient::passwordEntriesChanged,
            this, [this] { this->apiClient.getPasswordEntries(); });
    connect(&apiClient, &ApiClient::passwordCategoriesChanged, this, [this] {
        this->apiClient.getPasswordCategories();
        this->apiClient.getPasswordEntries();
    });
}

PasswordManagerView::~PasswordManagerView() {
    clearMasterPassword();
}

void PasswordManagerView::openCreateDialog() {
    if (masterPassword.isEmpty()) {
        pendingAction = PendingAction::CreateEntry;
        unlock();
        return;
    }
    openEntryDialog();
}

void PasswordManagerView::openCategoryManager() {
    if (masterPassword.isEmpty()) {
        pendingAction = PendingAction::ManageCategories;
        unlock();
        return;
    }
    openCategoryDialog();
}

void PasswordManagerView::unlock() {
    QDialog dialog(this);
    dialog.setWindowTitle("Passwortmanager entsperren");
    auto *form = new QFormLayout(&dialog);
    auto *passwordEdit = new QLineEdit(&dialog);
    passwordEdit->setEchoMode(QLineEdit::Password);
    form->addRow("Masterpasswort", passwordEdit);
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog
    );
    buttons->button(QDialogButtonBox::Ok)->setText("Entsperren");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);
    prepareDialog(dialog);
    passwordEdit->setFocus();
    if (execDialog(dialog) != QDialog::Accepted || passwordEdit->text().isEmpty()) {
        pendingAction = PendingAction::None;
        return;
    }

    masterPassword = passwordEdit->text().toUtf8();
    refresh();
}

void PasswordManagerView::clearMasterPassword() {
    if (!masterPassword.isEmpty()) {
        sodium_memzero(masterPassword.data(), static_cast<size_t>(masterPassword.size()));
    }
    masterPassword.clear();
    entries.clear();
}

void PasswordManagerView::refresh() {
    categoriesLoaded = false;
    entriesLoaded = false;
    apiClient.getPasswordCategories();
    apiClient.getPasswordEntries();
}

void PasswordManagerView::receiveEntries(const std::vector<PasswordEntry> &received) {
    if (masterPassword.isEmpty()) {
        return;
    }

    std::vector<DecryptedEntry> decrypted;
    for (const PasswordEntry &encrypted : received) {
        DecryptedEntry entry;
        if (!decryptEntry(encrypted, entry)) {
            QMessageBox::warning(
                this,
                "Passwortmanager",
                "Das Masterpasswort ist falsch oder ein Eintrag ist beschädigt."
            );
            clearMasterPassword();
            pendingAction = PendingAction::None;
            renderEntries();
            pages->setCurrentWidget(lockedPage);
            return;
        }
        decrypted.push_back(std::move(entry));
    }
    entries = std::move(decrypted);
    entriesLoaded = true;
    renderEntries();
    pages->setCurrentWidget(entriesPage);
    runPendingAction();
}

void PasswordManagerView::renderEntries() {
    while (QLayoutItem *item = cardLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    bool firstCard = true;
    for (const DecryptedEntry &entry : entries) {
        auto *card = new QFrame(entriesPage);
        card->setObjectName("passwordCard");
        card->setProperty("firstCard", firstCard);
        card->setFixedWidth(360);
        card->setAttribute(Qt::WA_Hover);
        card->setContextMenuPolicy(Qt::CustomContextMenu);
        card->setStyleSheet(R"(
            QFrame#passwordCard {
                background: #1C1C1C;
                border: none;
                border-bottom: 1px solid #383838;
                border-radius: 0;
            }
            QFrame#passwordCard[firstCard="true"] {
                border-top: 1px solid #383838;
            }
            QFrame#passwordCard:hover {
                background: #292929;
            }
            QFrame#passwordCard QLabel {
                border: none;
                background: transparent;
            }
        )");
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(14, 8, 14, 8);
        layout->setSpacing(2);
        auto *title = new QLabel(entry.title, card);
        QFont titleFont = title->font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 2);
        title->setFont(titleFont);
        title->setWordWrap(true);
        title->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(title);
        if (!entry.username.isEmpty()) {
            auto *username = new QLabel(entry.username, card);
            username->setWordWrap(true);
            username->setStyleSheet("color: #A8A8A8;");
            username->setAttribute(Qt::WA_TransparentForMouseEvents);
            layout->addWidget(username);
        }

        const int entryId = entry.encrypted.id;
        connect(card, &QWidget::customContextMenuRequested, card,
                [this, card, entryId](const QPoint &position) {
                    DecryptedEntry *selectedEntry = entryById(entryId);
                    if (!selectedEntry) {
                        return;
                    }
                    QMenu menu(card);
                    QAction *copyAction = menu.addAction("Passwort kopieren");
                    QAction *editAction = menu.addAction("Bearbeiten");
                    QAction *deleteAction = menu.addAction("Löschen");
                    QAction *selected = menu.exec(card->mapToGlobal(position));
                    if (selected == copyAction) {
                        QGuiApplication::clipboard()->setText(selectedEntry->password);
                    } else if (selected == editAction) {
                        openEntryDialog(selectedEntry);
                    } else if (selected == deleteAction) {
                        apiClient.deletePasswordEntry(entryId);
                    }
                });
        cardLayout->addWidget(card, 0, Qt::AlignHCenter);
        firstCard = false;
    }
    cardLayout->addStretch();
}

void PasswordManagerView::openEntryDialog(const DecryptedEntry *entry) {
    QDialog dialog(this);
    dialog.setWindowTitle(entry ? "Passworteintrag bearbeiten" : "Passworteintrag erstellen");
    auto *form = new QFormLayout(&dialog);
    auto *titleEdit = new QLineEdit(entry ? entry->title : QString(), &dialog);
    auto *usernameEdit = new QLineEdit(entry ? entry->username : QString(), &dialog);
    auto *passwordRow = new QWidget(&dialog);
    auto *passwordLayout = new QHBoxLayout(passwordRow);
    passwordLayout->setContentsMargins(0, 0, 0, 0);
    passwordLayout->setSpacing(6);
    auto *passwordEdit = new QLineEdit(entry ? entry->password : QString(), passwordRow);
    passwordEdit->setEchoMode(QLineEdit::Password);
    auto *showPasswordButton = new QPushButton("Anzeigen", passwordRow);
    showPasswordButton->setCheckable(true);
    passwordLayout->addWidget(passwordEdit, 1);
    passwordLayout->addWidget(showPasswordButton);
    connect(showPasswordButton, &QPushButton::toggled, &dialog,
            [passwordEdit, showPasswordButton](const bool visible) {
                passwordEdit->setEchoMode(
                    visible ? QLineEdit::Normal : QLineEdit::Password
                );
                showPasswordButton->setText(visible ? "Verbergen" : "Anzeigen");
            });
    auto *urlEdit = new QLineEdit(entry ? entry->url : QString(), &dialog);
    auto *noteEdit = new QTextEdit(entry ? entry->note : QString(), &dialog);
    auto *categoryEdit = new QComboBox(&dialog);
    categoryEdit->addItem("Ohne Kategorie");
    for (const PasswordCategory &category : categories) {
        categoryEdit->addItem(category.name, category.id);
    }
    if (entry && entry->encrypted.categoryId.has_value()) {
        categoryEdit->setCurrentIndex(
            categoryEdit->findData(*entry->encrypted.categoryId)
        );
    }
    form->addRow("Titel", titleEdit);
    form->addRow("Benutzername", usernameEdit);
    form->addRow("Passwort", passwordRow);
    form->addRow("URL", urlEdit);
    form->addRow("Notiz", noteEdit);
    form->addRow("Kategorie", categoryEdit);
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog
    );
    buttons->button(QDialogButtonBox::Save)->setText("Speichern");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);
    prepareDialog(dialog);
    if (execDialog(dialog) != QDialog::Accepted) {
        return;
    }

    DecryptedEntry changed{
        .title = titleEdit->text(),
        .username = usernameEdit->text(),
        .password = passwordEdit->text(),
        .url = urlEdit->text(),
        .note = noteEdit->toPlainText()
    };
    if (entry) {
        changed.encrypted.id = entry->encrypted.id;
    }
    const QVariant categoryId = categoryEdit->currentData();
    if (categoryId.isValid()) {
        changed.encrypted.categoryId = categoryId.toInt();
    }
    if (!encryptEntry(changed, changed.encrypted)) {
        QMessageBox::warning(this, "Passwortmanager", "Der Eintrag konnte nicht verschlüsselt werden.");
        return;
    }
    if (entry) {
        apiClient.updatePasswordEntry(
            changed.encrypted.id,
            changed.encrypted.categoryId,
            changed.encrypted.ciphertext,
            changed.encrypted.salt,
            changed.encrypted.nonce
        );
    } else {
        apiClient.createPasswordEntry(
            changed.encrypted.categoryId,
            changed.encrypted.ciphertext,
            changed.encrypted.salt,
            changed.encrypted.nonce
        );
    }
}

void PasswordManagerView::openCategoryDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Passwortkategorien");
    auto *layout = new QVBoxLayout(&dialog);
    auto *list = new QListWidget(&dialog);
    for (const PasswordCategory &category : categories) {
        auto *item = new QListWidgetItem(category.name, list);
        item->setData(Qt::UserRole, category.id);
    }
    layout->addWidget(list);
    auto *buttons = new QHBoxLayout;
    auto *createButton = new QPushButton("Neu", &dialog);
    auto *editButton = new QPushButton("Bearbeiten", &dialog);
    auto *deleteButton = new QPushButton("Löschen", &dialog);
    auto *closeButton = new QPushButton("Schließen", &dialog);
    buttons->addWidget(createButton);
    buttons->addWidget(editButton);
    buttons->addWidget(deleteButton);
    buttons->addStretch();
    buttons->addWidget(closeButton);
    layout->addLayout(buttons);

    connect(createButton, &QPushButton::clicked, &dialog, [this, &dialog] {
        bool accepted = false;
        const QString name = QInputDialog::getText(
            &dialog, "Kategorie erstellen", "Name", QLineEdit::Normal, QString(), &accepted
        ).trimmed();
        if (accepted && !name.isEmpty()) {
            apiClient.createPasswordCategory(name);
            dialog.accept();
        }
    });
    connect(editButton, &QPushButton::clicked, &dialog, [this, list, &dialog] {
        if (!list->currentItem()) {
            return;
        }
        bool accepted = false;
        const QString name = QInputDialog::getText(
            &dialog,
            "Kategorie bearbeiten",
            "Name",
            QLineEdit::Normal,
            list->currentItem()->text(),
            &accepted
        ).trimmed();
        if (accepted && !name.isEmpty()) {
            apiClient.updatePasswordCategory(
                list->currentItem()->data(Qt::UserRole).toInt(), name
            );
            dialog.accept();
        }
    });
    connect(deleteButton, &QPushButton::clicked, &dialog, [this, list, &dialog] {
        if (list->currentItem()) {
            apiClient.deletePasswordCategory(
                list->currentItem()->data(Qt::UserRole).toInt()
            );
            dialog.accept();
        }
    });
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    prepareDialog(dialog);
    execDialog(dialog);
}

PasswordManagerView::DecryptedEntry *PasswordManagerView::entryById(const int id) {
    const auto found = std::find_if(
        entries.begin(), entries.end(),
        [id](const DecryptedEntry &entry) { return entry.encrypted.id == id; }
    );
    return found == entries.end() ? nullptr : &*found;
}

void PasswordManagerView::runPendingAction() {
    if (!categoriesLoaded || !entriesLoaded) {
        return;
    }
    const PendingAction action = pendingAction;
    pendingAction = PendingAction::None;
    if (action == PendingAction::CreateEntry) {
        openEntryDialog();
    } else if (action == PendingAction::ManageCategories) {
        openCategoryDialog();
    }
}

bool PasswordManagerView::encryptEntry(
    const DecryptedEntry &entry,
    PasswordEntry &encrypted
) const {
    const QJsonDocument document(QJsonObject{
        {"title", entry.title},
        {"username", entry.username},
        {"password", entry.password},
        {"url", entry.url},
        {"note", entry.note}
    });
    const QByteArray plaintext = document.toJson(QJsonDocument::Compact);
    encrypted.salt.resize(crypto_pwhash_SALTBYTES);
    encrypted.nonce.resize(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
    randombytes_buf(encrypted.salt.data(), static_cast<size_t>(encrypted.salt.size()));
    randombytes_buf(encrypted.nonce.data(), static_cast<size_t>(encrypted.nonce.size()));

    unsigned char key[crypto_aead_xchacha20poly1305_ietf_KEYBYTES];
    if (crypto_pwhash(
            key,
            sizeof key,
            masterPassword.constData(),
            static_cast<unsigned long long>(masterPassword.size()),
            reinterpret_cast<const unsigned char *>(encrypted.salt.constData()),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE,
            crypto_pwhash_ALG_ARGON2ID13
        ) != 0) {
        return false;
    }

    encrypted.ciphertext.resize(
        plaintext.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES
    );
    unsigned long long ciphertextSize = 0;
    const int result = crypto_aead_xchacha20poly1305_ietf_encrypt(
        reinterpret_cast<unsigned char *>(encrypted.ciphertext.data()),
        &ciphertextSize,
        reinterpret_cast<const unsigned char *>(plaintext.constData()),
        static_cast<unsigned long long>(plaintext.size()),
        nullptr,
        0,
        nullptr,
        reinterpret_cast<const unsigned char *>(encrypted.nonce.constData()),
        key
    );
    sodium_memzero(key, sizeof key);
    encrypted.ciphertext.resize(static_cast<qsizetype>(ciphertextSize));
    return result == 0;
}

bool PasswordManagerView::decryptEntry(
    const PasswordEntry &encrypted,
    DecryptedEntry &entry
) const {
    if (encrypted.salt.size() != crypto_pwhash_SALTBYTES
        || encrypted.nonce.size() != crypto_aead_xchacha20poly1305_ietf_NPUBBYTES
        || encrypted.ciphertext.size() < crypto_aead_xchacha20poly1305_ietf_ABYTES) {
        return false;
    }

    unsigned char key[crypto_aead_xchacha20poly1305_ietf_KEYBYTES];
    if (crypto_pwhash(
            key,
            sizeof key,
            masterPassword.constData(),
            static_cast<unsigned long long>(masterPassword.size()),
            reinterpret_cast<const unsigned char *>(encrypted.salt.constData()),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE,
            crypto_pwhash_ALG_ARGON2ID13
        ) != 0) {
        return false;
    }

    QByteArray plaintext(
        encrypted.ciphertext.size() - crypto_aead_xchacha20poly1305_ietf_ABYTES,
        Qt::Uninitialized
    );
    unsigned long long plaintextSize = 0;
    const int result = crypto_aead_xchacha20poly1305_ietf_decrypt(
        reinterpret_cast<unsigned char *>(plaintext.data()),
        &plaintextSize,
        nullptr,
        reinterpret_cast<const unsigned char *>(encrypted.ciphertext.constData()),
        static_cast<unsigned long long>(encrypted.ciphertext.size()),
        nullptr,
        0,
        reinterpret_cast<const unsigned char *>(encrypted.nonce.constData()),
        key
    );
    sodium_memzero(key, sizeof key);
    if (result != 0) {
        return false;
    }
    plaintext.resize(static_cast<qsizetype>(plaintextSize));
    const QJsonDocument document = QJsonDocument::fromJson(plaintext);
    if (!document.isObject()) {
        return false;
    }
    const QJsonObject object = document.object();
    entry = {
        .encrypted = encrypted,
        .title = object["title"].toString(),
        .username = object["username"].toString(),
        .password = object["password"].toString(),
        .url = object["url"].toString(),
        .note = object["note"].toString()
    };
    return true;
}
