#ifndef FRONTEND_IDEAVIEW_H
#define FRONTEND_IDEAVIEW_H

#include "../api/ApiClient.h"

#include <QWidget>

#include <vector>

class QVBoxLayout;
class QPoint;

class IdeaView final : public QWidget {
public:
    explicit IdeaView(ApiClient &apiClient, QWidget *parent = nullptr);
    void refreshMetadata();

private:
    void renderIdeas();
    void renderDetail();
    void selectIdea(int id);
    void showCardContextMenu(int id, const QPoint &globalPosition);
    void openCreateDialog();
    void openEditDialog(const Idea &idea);
    Idea *ideaById(int id);

    ApiClient &apiClient;
    std::vector<Idea> ideas;
    std::vector<EntityFieldVersion> versions;
    int selectedIdeaId = 0;
    bool versionsLoading = false;
    QVBoxLayout *phaseLayout;
    QVBoxLayout *detailLayout;
};

#endif // FRONTEND_IDEAVIEW_H
