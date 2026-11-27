#ifndef FRONTEND_CONTEXTVIEW_H
#define FRONTEND_CONTEXTVIEW_H

#include "../models/Context.h"

#include <QWidget>

#include <optional>
#include <vector>

class ApiClient;
class QListWidget;
class QResizeEvent;

class ContextView final : public QWidget {
public:
    explicit ContextView(ApiClient &apiClient, QWidget *parent = nullptr);
    void openCreateDialog();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void renderContexts(const std::vector<Context> &contexts);
    void openContextDialog(std::optional<Context> context);

    ApiClient &apiClient;
    QListWidget *grid;
};

#endif // FRONTEND_CONTEXTVIEW_H
