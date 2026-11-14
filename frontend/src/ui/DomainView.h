#ifndef FRONTEND_DOMAINVIEW_H
#define FRONTEND_DOMAINVIEW_H

#include "../api/ApiClient.h"
#include "../models/Metadata.h"

#include <QHash>
#include <QWidget>

class QEvent;
class QFrame;
class QResizeEvent;
class QScrollArea;
class QVBoxLayout;

struct DomainCard {
    MetadataItem domain;
    QFrame *frame;
    bool isLoaded;
};

class DomainView : public QWidget {
public:
    explicit DomainView(QWidget *parent = nullptr);
    void renderDomains();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void renderDomainNodes(int domainId, const std::vector<DomainNode> &nodes);
    void updateCardWidths();

    ApiClient apiClient;
    QScrollArea *scrollArea;
    QVBoxLayout *cardsLayout;
    QHash<int, DomainCard> domainCards;
};

#endif // FRONTEND_DOMAINVIEW_H
