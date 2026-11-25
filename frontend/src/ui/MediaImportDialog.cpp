#include "MediaImportDialog.h"

#include "../api/ApiClient.h"
#include "DialogUtils.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

MediaImportDialog::MediaImportDialog(ApiClient &apiClient, QWidget *parent)
    : QDialog(parent), apiClient(apiClient) {
    setWindowTitle("Film oder Serie importieren");
    auto *layout = new QVBoxLayout(this);

    auto *urlLabel = new QLabel("&TMDB-Adresse", this);
    urlField = new QLineEdit(this);
    urlField->setPlaceholderText("https://www.themoviedb.org/…");
    urlField->setClearButtonEnabled(true);
    urlLabel->setBuddy(urlField);
    layout->addWidget(urlLabel);
    layout->addWidget(urlField);

    importStatus = new QLabel(this);
    importStatus->setWordWrap(true);
    importStatus->hide();
    layout->addWidget(importStatus);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch(1);
    cancelButton = new QPushButton("Abbrechen", this);
    cancelButton->setAutoDefault(false);
    buttons->addWidget(cancelButton);
    importButton = new QPushButton("Importieren", this);
    importButton->setAutoDefault(false);
    buttons->addWidget(importButton);
    layout->addLayout(buttons);

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

    prepareDialog(*this);
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
