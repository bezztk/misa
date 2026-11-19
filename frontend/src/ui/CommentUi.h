#ifndef FRONTEND_COMMENTUI_H
#define FRONTEND_COMMENTUI_H

#include "../models/Comment.h"

#include <QStyledItemDelegate>

#include <functional>
#include <vector>

class ApiClient;
class QWidget;

inline constexpr int COMMENT_COUNT_ROLE = Qt::UserRole + 50;
inline constexpr int COMMENT_ENTITY_ID_ROLE = Qt::UserRole + 51;

class CommentIndicatorDelegate final : public QStyledItemDelegate {
public:
    explicit CommentIndicatorDelegate(
        std::function<void(int)> openComments,
        std::function<void()> indicatorPressed = {},
        QObject *parent = nullptr
    );

    void paint(
        QPainter *painter,
        const QStyleOptionViewItem &option,
        const QModelIndex &index
    ) const override;
    bool editorEvent(
        QEvent *event,
        QAbstractItemModel *model,
        const QStyleOptionViewItem &option,
        const QModelIndex &index
    ) override;

private:
    std::function<void(int)> openComments;
    std::function<void()> indicatorPressed;
};

void showCommentsDialog(
    QWidget *parent,
    ApiClient &apiClient,
    int entityId,
    const QString &title,
    std::vector<Comment> &comments
);

#endif // FRONTEND_COMMENTUI_H
