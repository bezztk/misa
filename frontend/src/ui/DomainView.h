#ifndef FRONTEND_DOMAINVIEW_H
#define FRONTEND_DOMAINVIEW_H

#include <QWidget>

#include <vector>

class QFrame;
class QResizeEvent;
class QScrollArea;
class QVBoxLayout;

class DomainView : public QWidget {
public:
    explicit DomainView(QWidget *parent = nullptr);
    void renderDomains();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateCardWidths();

    QScrollArea *scrollArea;
    QVBoxLayout *cardsLayout;
    std::vector<QFrame *> domainCards;
};

#endif // FRONTEND_DOMAINVIEW_H
