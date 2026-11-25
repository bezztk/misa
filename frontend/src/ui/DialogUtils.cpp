#include "DialogUtils.h"

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QEventLoop>
#include <QFormLayout>
#include <QInputDialog>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

namespace {
class OutsideClickFilter final : public QObject {
public:
    OutsideClickFilter(QDialog &dialog, const bool acceptOnOutsideClick)
        : dialog(dialog), acceptOnOutsideClick(acceptOnOutsideClick) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() != QEvent::MouseButtonPress) {
            return false;
        }

        auto *widget = qobject_cast<QWidget *>(watched);
        if (!widget || belongsToDialog(widget)
            || widget->window()->windowType() == Qt::Popup) {
            return false;
        }

        acceptOnOutsideClick ? dialog.accept() : dialog.reject();
        return true;
    }

private:
    bool belongsToDialog(QWidget *widget) const {
        for (QWidget *current = widget; current; current = current->parentWidget()) {
            if (current == &dialog) {
                return true;
            }
        }
        return false;
    }

    QDialog &dialog;
    bool acceptOnOutsideClick;
};
}

void prepareDialog(QDialog &dialog, const int minimumWidth) {
    dialog.setMinimumWidth(minimumWidth);
    if (dialog.layout()) {
        dialog.layout()->setContentsMargins(20, 20, 20, 20);
        dialog.layout()->setSpacing(14);
    }

    const auto forms = dialog.findChildren<QFormLayout *>();
    for (QFormLayout *form : forms) {
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        form->setHorizontalSpacing(18);
        form->setVerticalSpacing(12);
    }

    const auto lineEdits = dialog.findChildren<QLineEdit *>();
    for (QLineEdit *edit : lineEdits) {
        edit->setMinimumHeight(28);
    }
    const auto comboBoxes = dialog.findChildren<QComboBox *>();
    for (QComboBox *combo : comboBoxes) {
        combo->setMinimumHeight(28);
    }
    const auto buttons = dialog.findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        button->setMinimumSize(90, 30);
    }

    if (dialog.layout()) {
        dialog.layout()->activate();
        dialog.resize(dialog.size().expandedTo(dialog.minimumSizeHint()));
    }
}

int execDialog(QDialog &dialog, const bool acceptOnOutsideClick) {
    OutsideClickFilter outsideClickFilter(dialog, acceptOnOutsideClick);
    QApplication::instance()->installEventFilter(&outsideClickFilter);

    QEventLoop eventLoop;
    QObject::connect(&dialog, &QDialog::finished, &eventLoop, &QEventLoop::quit);
    dialog.setWindowModality(Qt::NonModal);
    dialog.show();
    dialog.raise();
    dialog.activateWindow();
    eventLoop.exec(QEventLoop::DialogExec);

    QApplication::instance()->removeEventFilter(&outsideClickFilter);
    return dialog.result();
}

std::optional<QString> showTextInputDialog(
    QWidget *parent,
    const QString &title,
    const QString &label,
    const QString &acceptText,
    const QString &initialValue
) {
    QInputDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setTextValue(initialValue);
    dialog.setOkButtonText(acceptText);
    dialog.setCancelButtonText("Abbrechen");
    prepareDialog(dialog);

    if (execDialog(dialog) != QDialog::Accepted) {
        return std::nullopt;
    }
    return dialog.textValue();
}

std::optional<QString> showItemInputDialog(
    QWidget *parent,
    const QString &title,
    const QString &label,
    const QStringList &items,
    const QString &acceptText
) {
    QInputDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setComboBoxItems(items);
    dialog.setComboBoxEditable(false);
    dialog.setOkButtonText(acceptText);
    dialog.setCancelButtonText("Abbrechen");
    prepareDialog(dialog);

    if (execDialog(dialog) != QDialog::Accepted) {
        return std::nullopt;
    }
    return dialog.textValue();
}
