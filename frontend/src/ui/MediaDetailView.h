#ifndef FRONTEND_MEDIADETAILVIEW_H
#define FRONTEND_MEDIADETAILVIEW_H

#include <QScrollArea>

class QLabel;
class MediaDetailHero;
struct Media;

class MediaDetailView final : public QScrollArea {
    Q_OBJECT

public:
    explicit MediaDetailView(QWidget *parent = nullptr);
    void setMedia(const Media &media, const QString &posterFile, const QString &backdropFile);

signals:
    void backRequested();

private:
    MediaDetailHero *hero;
    QLabel *cover;
    QLabel *title;
    QLabel *metadata;
    QLabel *description;
};

#endif // FRONTEND_MEDIADETAILVIEW_H
