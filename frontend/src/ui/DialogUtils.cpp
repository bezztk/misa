#include "DialogUtils.h"

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QEventLoop>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {
constexpr auto formStyle = R"(
    QLabel[formLabel="true"] {
        color: #D2D2D2;
        font-family: "Bahnschrift";
        font-size: 13px;
        font-weight: 500;
    }
    QLineEdit[formField="true"], QPlainTextEdit[formField="true"], QTextEdit[formField="true"] {
        background: #2B2B2B;
        color: #F4F4F4;
        border: 1px solid #5E5E5E;
        border-radius: 0;
        padding: 8px 12px;
        font-family: "Bahnschrift";
        font-size: 14px;
        selection-background-color: #626262;
        selection-color: #FFFFFF;
    }
    QLineEdit[formField="true"] { padding: 0 12px; }
    QLineEdit[formField="true"]:focus, QPlainTextEdit[formField="true"]:focus,
    QTextEdit[formField="true"]:focus { border-color: #B8B8B8; }
    QFrame#formDialogFooter { border-top: 1px solid #414141; }
    QDialogButtonBox#formDialogActions { button-layout: 3; }
    QPushButton[formAction="true"] {
        min-height: 36px;
        min-width: 60px;
        padding: 0 15px;
        font-family: "Bahnschrift";
        font-size: 14px;
        font-weight: 500;
        color: #F4F4F4;
        background: transparent;
        border: 1px solid #5E5E5E;
        border-radius: 0;
    }
    QPushButton[formAction="true"]:hover { background: #353535; }
    QPushButton[formAction="true"]:pressed { background: #414141; }
    QPushButton[formPrimary="true"] {
        min-width: 80px;
        color: #202020;
        background: #D2D2D2;
        border-color: #D2D2D2;
    }
    QPushButton[formPrimary="true"]:hover { background: #E6E6E6; }
    QPushButton[formPrimary="true"]:pressed { background: #B8B8B8; }
    QPushButton[formAction="true"]:focus { border-color: #FFFFFF; }
    QPushButton[formAction="true"]:disabled {
        color: #929292;
        background: transparent;
        border-color: #414141;
    }
)";

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

void prepareFormDialog(QDialog &dialog, QDialogButtonBox &buttons, const int minimumWidth) {
    dialog.setMinimumWidth(minimumWidth);
    auto *content = new QWidget(&dialog);
    content->setLayout(dialog.layout());
    content->layout()->setContentsMargins(20, 20, 20, 16);
    content->layout()->setSpacing(14);

    for (auto *form : content->findChildren<QFormLayout *>()) {
        form->setRowWrapPolicy(QFormLayout::WrapAllRows);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        form->setVerticalSpacing(8);
        for (int row = 0; row < form->rowCount(); ++row) {
            if (auto *label = form->itemAt(row, QFormLayout::LabelRole)) {
                label->widget()->setProperty("formLabel", true);
            }
        }
    }
    for (auto *field : content->findChildren<QLineEdit *>()) {
        field->setProperty("formField", true);
        field->setMinimumHeight(38);
    }
    for (auto *field : content->findChildren<QPlainTextEdit *>()) {
        field->setProperty("formField", true);
    }
    for (auto *field : content->findChildren<QTextEdit *>()) {
        field->setProperty("formField", true);
    }
    for (auto *button : content->findChildren<QPushButton *>()) {
        button->setProperty("formAction", true);
    }

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(content, 1);

    auto *footer = new QFrame(&dialog);
    footer->setObjectName("formDialogFooter");
    auto *footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(20, 12, 20, 16);
    buttons.setObjectName("formDialogActions");
    for (auto *button : buttons.buttons()) {
        button->setProperty("formAction", true);
        if (buttons.buttonRole(button) == QDialogButtonBox::AcceptRole) {
            button->setProperty("formPrimary", true);
        }
    }
    footerLayout->addWidget(&buttons);
    layout->addWidget(footer);
    dialog.setStyleSheet(formStyle);
    layout->activate();
    dialog.resize(dialog.size().expandedTo(dialog.minimumSizeHint()));
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
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    auto *form = new QFormLayout(&dialog);
    auto *field = new QLineEdit(initialValue, &dialog);
    form->addRow(label, field);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(acceptText);
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    prepareFormDialog(dialog, *buttons);
    field->setFocus();
    field->selectAll();

    if (execDialog(dialog) != QDialog::Accepted) {
        return std::nullopt;
    }
    return field->text();
}

std::optional<QString> showItemInputDialog(
    QWidget *parent,
    const QString &title,
    const QString &label,
    const QStringList &items,
    const QString &acceptText
) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    auto *form = new QFormLayout(&dialog);
    auto *field = new QComboBox(&dialog);
    field->addItems(items);
    form->addRow(label, field);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(acceptText);
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    prepareFormDialog(dialog, *buttons);
    field->setFocus();

    if (execDialog(dialog) != QDialog::Accepted) {
        return std::nullopt;
    }
    return field->currentText();
}
