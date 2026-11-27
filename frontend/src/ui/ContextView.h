#ifndef FRONTEND_CONTEXTVIEW_H
#define FRONTEND_CONTEXTVIEW_H

#include <QWidget>

class ApiClient;

class ContextView final : public QWidget {
public:
    explicit ContextView(ApiClient &apiClient, QWidget *parent = nullptr);
};

#endif // FRONTEND_CONTEXTVIEW_H
