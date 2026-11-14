#ifndef FRONTEND_MAINWINDOW_H
#define FRONTEND_MAINWINDOW_H

#include <QMainWindow>
#include "../api/ApiClient.h"

class DomainView;
class QCloseEvent;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    ApiClient apiClient;
    DomainView *domainView;
    bool closeAfterSave = false;
};

#endif //FRONTEND_MAINWINDOW_H
