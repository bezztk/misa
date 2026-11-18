#ifndef FRONTEND_MAINWINDOW_H
#define FRONTEND_MAINWINDOW_H

#include <QMainWindow>
#include "../api/ApiClient.h"

class DomainView;
class QCloseEvent;
class QLabel;
class QResizeEvent;
class QTimer;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void showToast(const QString &message);
    void positionToast();

    ApiClient apiClient;
    DomainView *domainView;
    QLabel *toastLabel;
    QTimer *toastTimer;
    bool closeAfterSave = false;
};

#endif //FRONTEND_MAINWINDOW_H
