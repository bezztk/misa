#ifndef FRONTEND_IDEAVIEW_H
#define FRONTEND_IDEAVIEW_H

#include "../api/ApiClient.h"

#include <QWidget>

#include <vector>

class QVBoxLayout;

class IdeaView final : public QWidget {
public:
    explicit IdeaView(ApiClient &apiClient, QWidget *parent = nullptr);
    void refreshMetadata();
    void openCreateDialog();

private:
    void renderIdeas();
    QWidget *createCard(const Idea &idea, QWidget *parent);
    Idea *ideaById(int id);
    void openEditDialog(int id);
    void openComments(int id);

    ApiClient &apiClient;
    std::vector<Idea> ideas;
    std::vector<Context> contexts;
    QVBoxLayout *phaseLayout;
};

#endif // FRONTEND_IDEAVIEW_H
