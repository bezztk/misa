#ifndef FRONTEND_MEDIAIMPORTDIALOG_H
#define FRONTEND_MEDIAIMPORTDIALOG_H

#include <QDialog>

class ApiClient;
class QLabel;
class QLineEdit;
class QPushButton;
class QWidget;

class MediaImportDialog final : public QDialog {
public:
    explicit MediaImportDialog(ApiClient &apiClient, QWidget *parent = nullptr);
    void reject() override;

private:
    void importUrl();
    void updateButtons();

    ApiClient &apiClient;
    QLineEdit *urlField;
    QLabel *importStatus;
    QPushButton *cancelButton;
    QPushButton *importButton;
    int importRequestId = 0;
};

#endif // FRONTEND_MEDIAIMPORTDIALOG_H
