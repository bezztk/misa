#ifndef FRONTEND_PASSWORD_H
#define FRONTEND_PASSWORD_H

#include <QByteArray>
#include <QString>

#include <optional>

struct PasswordEntry {
    int id;
    std::optional<int> categoryId;
    QByteArray ciphertext;
    QByteArray salt;
    QByteArray nonce;
};

struct PasswordCategory {
    int id;
    QString name;
};

#endif // FRONTEND_PASSWORD_H
