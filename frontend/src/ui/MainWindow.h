#ifndef FRONTEND_MAINWINDOW_H
#define FRONTEND_MAINWINDOW_H

#include <QMainWindow>
#include "../api/ApiClient.h"

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    ApiClient apiClient;
};

#endif //FRONTEND_MAINWINDOW_H
