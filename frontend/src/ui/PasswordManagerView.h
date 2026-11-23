#ifndef FRONTEND_PASSWORDMANAGERVIEW_H
#define FRONTEND_PASSWORDMANAGERVIEW_H

#include "../models/Password.h"

#include <QByteArray>
#include <QWidget>

#include <vector>

class ApiClient;
class QStackedWidget;
class QVBoxLayout;

class PasswordManagerView final : public QWidget {
    Q_OBJECT

public:
    explicit PasswordManagerView(ApiClient &apiClient, QWidget *parent = nullptr);
    ~PasswordManagerView() override;
    void openCreateDialog();
    void openCategoryManager();

private:
    struct DecryptedEntry {
        PasswordEntry encrypted;
        QString title;
        QString username;
        QString password;
        QString url;
        QString note;
    };

    void unlock();
    void clearMasterPassword();
    void refresh();
    void renderEntries();
    void receiveEntries(const std::vector<PasswordEntry> &received);
    void openEntryDialog(const DecryptedEntry *entry = nullptr);
    void openCategoryDialog();
    DecryptedEntry *entryById(int id);
    void runPendingAction();
    bool encryptEntry(const DecryptedEntry &entry, PasswordEntry &encrypted) const;
    bool decryptEntry(const PasswordEntry &encrypted, DecryptedEntry &entry) const;

    ApiClient &apiClient;
    QStackedWidget *pages;
    QWidget *lockedPage;
    QWidget *entriesPage;
    QVBoxLayout *cardLayout;
    QByteArray masterPassword;
    std::vector<PasswordCategory> categories;
    std::vector<DecryptedEntry> entries;
    enum class PendingAction { None, CreateEntry, ManageCategories };
    PendingAction pendingAction = PendingAction::None;
    bool categoriesLoaded = false;
    bool entriesLoaded = false;
};

#endif // FRONTEND_PASSWORDMANAGERVIEW_H
