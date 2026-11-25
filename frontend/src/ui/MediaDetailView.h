#ifndef FRONTEND_MEDIADETAILVIEW_H
#define FRONTEND_MEDIADETAILVIEW_H

#include "../models/Media.h"

#include <QScrollArea>

class ApiClient;
class QComboBox;
class QLabel;
class MediaDetailHero;

class MediaDetailView final : public QScrollArea {
    Q_OBJECT

public:
    explicit MediaDetailView(ApiClient &apiClient, QWidget *parent = nullptr);
    void setMedia(const Media &media, const QString &posterFile, const QString &backdropFile);

signals:
    void backRequested();

private:
    void setRating(std::optional<MediaRating> value);

    MediaDetailHero *hero;
    QLabel *cover;
    QLabel *title;
    QLabel *metadata;
    QLabel *description;
    QComboBox *rating;
    int entityId = 0;
    int ratingUpdateEntityId = 0;
    std::optional<MediaRating> savedRating;
};

#endif // FRONTEND_MEDIADETAILVIEW_H
