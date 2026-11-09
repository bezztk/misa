#ifndef FRONTEND_APICLIENT_H
#define FRONTEND_APICLIENT_H
#include "../models/Task.h"
#include <QNetworkAccessManager>

class ApiClient : public QObject {
    Q_OBJECT

signals:
    void taskReceived(const std::vector<Task>& tasks);

public:
    ApiClient();
    void getTasks();

private:
    static constexpr auto SCHEME = "http";
    static constexpr auto HOST = "localhost";
    static constexpr int PORT = 3000;
    static constexpr auto BASE_PATH = "/api";
    static constexpr auto PATH_TASKS = "/tasks";

    static QUrl buildUrl(const QString& path);

    QNetworkAccessManager manager;
};

#endif //FRONTEND_APICLIENT_H
