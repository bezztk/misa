#ifndef FRONTEND_DOMAINVIEW_H
#define FRONTEND_DOMAINVIEW_H

#include "../api/ApiClient.h"
#include "../models/Metadata.h"

#include <QHash>
#include <QWidget>

class QEvent;
class QFrame;
class QHBoxLayout;
class QPoint;
class QResizeEvent;
class QScrollArea;
class QShowEvent;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QVBoxLayout;

struct DomainCard {
    MetadataItem domain;
    QFrame *frame;
    QTreeView *treeView;
    QStandardItemModel *model;
    bool isLoaded;
};

class DomainView : public QWidget {
public:
    explicit DomainView(QWidget *parent = nullptr);
    void renderDomains();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void renderDomainNodes(int domainId, const std::vector<DomainNode> &nodes);
    void showDomainNodeContextMenu(int domainId, const QPoint &position);
    QStandardItem *findDomainNodeItem(int domainId, int nodeId);
    void updateTreeHeight(QTreeView *treeView);
    void updateCardLayout();

    ApiClient apiClient;
    QScrollArea *scrollArea;
    QWidget *cardsWidget;
    QWidget *secondColumn;
    QHBoxLayout *cardsLayout;
    QVBoxLayout *firstColumnLayout;
    QVBoxLayout *secondColumnLayout;
    QHash<int, DomainCard> domainCards;
};

#endif // FRONTEND_DOMAINVIEW_H
