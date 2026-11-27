#include "MediaImportDialog.h"

#include "../api/ApiClient.h"
#include "DialogUtils.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

MediaImportDialog::MediaImportDialog(ApiClient &apiClient, QWidget *parent)
    : QDialog(parent), apiClient(apiClient) {
    setWindowTitle("Film oder Serie importieren");
    auto *layout = new QFormLayout(this);

    urlField = new QLineEdit(this);
    urlField->setPlaceholderText("https://www.themoviedb.org/…");
    urlField->setClearButtonEnabled(true);
    layout->addRow("&TMDB-Adresse", urlField);

    importStatus = new QLabel(this);
    importStatus->setWordWrap(true);
    importStatus->hide();
    layout->addRow(importStatus);

    auto *buttons = new QDialogButtonBox(this);
    cancelButton = buttons->addButton("Abbrechen", QDialogButtonBox::RejectRole);
    cancelButton->setAutoDefault(false);
    importButton = buttons->addButton("Importieren", QDialogButtonBox::AcceptRole);
    importButton->setAutoDefault(false);

    connect(urlField, &QLineEdit::textChanged, this, &MediaImportDialog::updateButtons);
    connect(urlField, &QLineEdit::returnPressed, importButton, &QPushButton::click);
    connect(importButton, &QPushButton::clicked, this, &MediaImportDialog::importUrl);
    connect(cancelButton, &QPushButton::clicked, this, &MediaImportDialog::reject);

    connect(&apiClient, &ApiClient::mediaImported, this, [this](const int id) {
        if (id == importRequestId) {
            importRequestId = 0;
            accept();
        }
    });
    connect(&apiClient, &ApiClient::mediaImportFailed, this,
            [this](const int id, const QString &message) {
                if (id == importRequestId) {
                    importRequestId = 0;
                    importStatus->setText(message);
                    updateButtons();
                }
            });

    prepareFormDialog(*this, *buttons);
    updateButtons();
    urlField->setFocus();
}

void MediaImportDialog::importUrl() {
    importRequestId = apiClient.importMedia(urlField->text().trimmed());
    importStatus->setText("Medium wird importiert …");
    importStatus->show();
    updateButtons();
}

void MediaImportDialog::updateButtons() {
    const bool importing = importRequestId != 0;
    urlField->setEnabled(!importing);
    cancelButton->setEnabled(!importing);
    importButton->setEnabled(!importing && !urlField->text().trimmed().isEmpty());
}

void MediaImportDialog::reject() {
    if (importRequestId == 0) {
        QDialog::reject();
    }
}
