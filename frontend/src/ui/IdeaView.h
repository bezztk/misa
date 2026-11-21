#ifndef FRONTEND_IDEAVIEW_H
#define FRONTEND_IDEAVIEW_H

#include "../api/ApiClient.h"

#include <QWidget>

#include <vector>

class QVBoxLayout;
class QHBoxLayout;
class QPoint;
class QResizeEvent;

class IdeaView final : public QWidget {
public:
    explicit IdeaView(ApiClient &apiClient, QWidget *parent = nullptr);
    void refreshMetadata();
    void openCreateDialog();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void renderIdeas();
    void renderDetail();
    void updateDetailVisibility();
    void selectIdea(int id);
    void showCardContextMenu(int id, const QPoint &globalPosition);
    void openEditDialog(const Idea &idea);
    void openPhaseDialog(const Idea &idea);
    Idea *ideaById(int id);

    ApiClient &apiClient;
    std::vector<Idea> ideas;
    std::vector<EntityFieldVersion> versions;
    int selectedIdeaId = 0;
    bool versionsLoading = false;
    QWidget *content;
    QWidget *body;
    QWidget *detail = nullptr;
    QHBoxLayout *bodyLayout;
    QVBoxLayout *phaseLayout;
    QVBoxLayout *detailLayout = nullptr;
};

#endif // FRONTEND_IDEAVIEW_H
