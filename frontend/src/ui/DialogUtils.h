#ifndef FRONTEND_DIALOGUTILS_H
#define FRONTEND_DIALOGUTILS_H

#include <QString>
#include <QStringList>

#include <optional>

class QDialog;
class QDialogButtonBox;
class QWidget;

void prepareDialog(QDialog &dialog, int minimumWidth = 460);
void prepareFormDialog(QDialog &dialog, QDialogButtonBox &buttons, int minimumWidth = 460);
int execDialog(QDialog &dialog, bool acceptOnOutsideClick = false);

std::optional<QString> showTextInputDialog(
    QWidget *parent,
    const QString &title,
    const QString &label,
    const QString &acceptText = "Erstellen",
    const QString &initialValue = {}
);

std::optional<QString> showItemInputDialog(
    QWidget *parent,
    const QString &title,
    const QString &label,
    const QStringList &items,
    const QString &acceptText
);

#endif // FRONTEND_DIALOGUTILS_H
