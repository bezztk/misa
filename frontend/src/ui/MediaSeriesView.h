#ifndef FRONTEND_MEDIASERIESVIEW_H
#define FRONTEND_MEDIASERIESVIEW_H

#include "../models/Media.h"

#include <QWidget>

#include <vector>

class ApiClient;
class QLabel;
class MediaSeasonCard;
class QProgressBar;
class QVBoxLayout;

class MediaSeriesView final : public QWidget {
public:
    explicit MediaSeriesView(ApiClient &apiClient, QWidget *parent);
    void setMedia(const Media &media);

private:
    void loadProgress();
    void loadEpisodes(MediaSeasonCard *card);
    void showSeasons(const std::vector<MediaSeason> &seasons);
    void showProgress(const MediaSeriesProgress &progress);

    ApiClient &apiClient;
    Media series{};
    QLabel *progressNotice;
    QLabel *regularLabel;
    QLabel *specialsLabel;
    QProgressBar *regularProgress;
    QLabel *seasonNotice;
    QVBoxLayout *seasonLayout;
    std::vector<MediaSeasonCard *> cards;
    int seasonsRequestId = 0;
    int progressRequestId = 0;
};

#endif // FRONTEND_MEDIASERIESVIEW_H
