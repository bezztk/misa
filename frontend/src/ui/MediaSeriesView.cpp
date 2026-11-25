#include "MediaSeriesView.h"

#include "../api/ApiClient.h"
#include "MediaImages.h"
#include "MediaUi.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QPushButton>

#include <algorithm>

namespace {
void setProgressBar(QProgressBar &bar, const double percentage) {
    bar.setValue(qRound(percentage));
}

QProgressBar *progressBar(QWidget *parent) {
    auto *bar = new QProgressBar(parent);
    bar->setRange(0, 100);
    bar->setFormat("%p %");
    return bar;
}

class MediaEpisodeRow final : public QFrame {
public:
    MediaEpisodeRow(ApiClient &apiClient, const Media &media, const MediaSeason &season,
                    const MediaEpisode &episode, QWidget *parent)
        : QFrame(parent), episodeId(episode.id) {
        setObjectName("mediaEpisode");
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(12);
        picture = new MediaImage(this);
        picture->setFixedSize(180, 101);
        picture->setFocusPolicy(Qt::NoFocus);
        picture->setAttribute(Qt::WA_TransparentForMouseEvents);
        const QString imageName = QString("seasons/%1/episodes/%2/still").arg(season.number).arg(episode.number);
        picture->setFile(mediaImageFile(mediaImagePath(media, imageName, episode.stillPath)),
                         QSize(360, 202));
        layout->addWidget(picture, 0, Qt::AlignTop);

        auto *details = new QWidget(this);
        auto *text = new QVBoxLayout(details);
        text->setContentsMargins(0, 0, 0, 0);
        text->setSpacing(5);
        layout->addWidget(details, 1);
        auto *heading = new QHBoxLayout();
        heading->setSpacing(8);
        QString title = QString("E%1").arg(episode.number, 2, 10, QChar('0'));
        if (!episode.title.isEmpty()) {
            title += " · " + episode.title;
        }
        auto *titleLabel = mediaText(title, this);
        titleLabel->setObjectName("mediaEpisodeTitle");
        heading->addWidget(titleLabel, 1);
        markWatched = new QPushButton(this);
        markWatched->setObjectName("mediaEpisodeWatchedButton");
        markWatched->setCheckable(true);
        markWatched->setCursor(Qt::PointingHandCursor);
        heading->addWidget(markWatched, 0, Qt::AlignTop);
        text->addLayout(heading);
        QStringList facts;
        if (episode.runtimeMinutes) {
            facts.append(QString("%1 Min.").arg(*episode.runtimeMinutes));
        }
        if (episode.releaseDate.isValid()) {
            facts.append(episode.releaseDate.toString("dd.MM.yyyy"));
        }
        if (!facts.isEmpty()) {
            auto *factsLabel = mediaText(facts.join(" · "), this);
            factsLabel->setObjectName("mediaMutedText");
            text->addWidget(factsLabel);
        }
        auto *description = new MediaDescription(260, this);
        description->setText(episode.description);
        text->addWidget(description);
        watchedAt = mediaText({}, this);
        watchedAt->setObjectName("mediaWatchedAt");
        text->addWidget(watchedAt);
        text->addStretch(1);
        setWatchState(episode);

        connect(markWatched, &QPushButton::clicked, this,
                [this, &apiClient, mediaId = media.entityId, seasonId = season.id](const bool watched) {
                    markWatched->setChecked(isWatched);
                    markWatched->setEnabled(false);
                    requestId = apiClient.updateMediaEpisodeWatched(mediaId, seasonId, episodeId, watched);
                });
        connect(&apiClient, &ApiClient::mediaEpisodeWatchedUpdateFinished, this,
                [this](const int id, int, int, const bool successful) {
                    if (id != requestId) {
                        return;
                    }
                    requestId = 0;
                    if (successful) {
                        isWatched = !isWatched;
                        updateWatchButton();
                        watchedAt->hide();
                    }
                    markWatched->setEnabled(true);
                });
    }

    int id() const { return episodeId; }

    void setWatchState(const MediaEpisode &episode) {
        if (requestId != 0) {
            return;
        }
        isWatched = episode.isWatched;
        updateWatchButton();
        watchedAt->setVisible(isWatched && episode.watchedAt.isValid());
        watchedAt->setText(QString("Gesehen am %1")
            .arg(episode.watchedAt.toLocalTime().toString("dd.MM.yyyy 'um' HH:mm")));
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QFrame::resizeEvent(event);
        const int imageWidth = width() < 430 ? 92 : width() < 520 ? 112 : 180;
        picture->setFixedSize(imageWidth, imageWidth * 9 / 16);
    }

private:
    void updateWatchButton() {
        const QString label = isWatched ? QStringLiteral("✓ Gesehen") : QStringLiteral("Als gesehen markieren");
        markWatched->setChecked(isWatched);
        markWatched->setText(label);
        markWatched->setAccessibleName(isWatched ? "Markierung als gesehen aufheben" : label);
        markWatched->setToolTip(markWatched->accessibleName());
    }

    int episodeId;
    bool isWatched = false;
    int requestId = 0;
    MediaImage *picture;
    QPushButton *markWatched;
    QLabel *watchedAt;
};
}

class MediaSeasonCard final : public QFrame {
public:
    MediaSeasonCard(const Media &media, const MediaSeason &season, QWidget *parent)
        : QFrame(parent), season(season) {
        setObjectName("mediaSeason");
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(9);

        auto *header = new QHBoxLayout();
        header->setSpacing(12);
        auto *poster = new MediaImage(this);
        poster->setFixedSize(64, 96);
        poster->setFocusPolicy(Qt::NoFocus);
        poster->setAttribute(Qt::WA_TransparentForMouseEvents);
        const QString posterName = QString("seasons/%1/poster").arg(season.number);
        poster->setFile(mediaImageFile(mediaImagePath(media, posterName, season.posterPath)),
                        QSize(128, 192));
        header->addWidget(poster, 0, Qt::AlignTop);

        auto *details = new QWidget(this);
        auto *text = new QVBoxLayout(details);
        text->setContentsMargins(0, 0, 0, 0);
        text->setSpacing(5);
        const QString name = season.number == 0 ? QStringLiteral("Sonderfolgen")
                                                : QString("Staffel %1").arg(season.number);
        auto *nameLabel = mediaText(name, details);
        nameLabel->setObjectName("mediaSeasonTitle");
        text->addWidget(nameLabel);
        toggle = new QToolButton(this);
        toggle->setObjectName("mediaSeasonToggle");
        toggle->setAccessibleName(name + " ein- oder ausklappen");
        toggle->setCheckable(true);
        toggle->setArrowType(Qt::RightArrow);
        toggle->setCursor(Qt::PointingHandCursor);
        if (!season.title.isEmpty()) {
            text->addWidget(mediaText(season.title, this));
        }
        if (season.releaseDate.isValid()) {
            auto *date = mediaText(season.releaseDate.toString("dd.MM.yyyy"), this);
            date->setObjectName("mediaMutedText");
            text->addWidget(date);
        }
        count = mediaText({}, this);
        count->setObjectName("mediaMutedText");
        text->addWidget(count);
        auto *description = new MediaDescription(300, this);
        description->setText(season.description);
        text->addWidget(description);
        header->addWidget(details, 1, Qt::AlignTop);
        header->addWidget(toggle, 0, Qt::AlignTop);
        layout->addLayout(header);

        auto *episodes = new QWidget(this);
        episodeLayout = new QVBoxLayout(episodes);
        episodeLayout->setContentsMargins(0, 4, 0, 0);
        episodeLayout->setSpacing(8);
        episodeNotice = mediaText({}, episodes);
        episodeNotice->setObjectName("mediaMutedText");
        episodeLayout->addWidget(episodeNotice);
        layout->addWidget(episodes);
        episodes->hide();
        connect(toggle, &QToolButton::toggled, this, [this, episodes](const bool open) {
            toggle->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
            episodes->setVisible(open);
        });
        setCounts(season);
    }

    void setCounts(const MediaSeason &updated) {
        season.episodeCount = updated.episodeCount;
        season.watchedEpisodeCount = updated.watchedEpisodeCount;
        if (season.episodeCount == 0 || season.watchedEpisodeCount == 0) {
            count->setText("Ausstehend");
        } else if (season.watchedEpisodeCount == season.episodeCount) {
            count->setText("Vollständig gesehen");
        } else {
            count->setText(QString("%1 von %2 Episoden gesehen")
                .arg(season.watchedEpisodeCount).arg(season.episodeCount));
        }
    }

    void showEpisodes(ApiClient &apiClient, const Media &media, const std::vector<MediaEpisode> &episodes) {
        episodesLoaded = true;
        const bool sameEpisodes = episodes.size() == rows.size()
            && std::equal(episodes.begin(), episodes.end(), rows.begin(),
                [](const MediaEpisode &episode, const MediaEpisodeRow *row) {
                    return episode.id == row->id();
                });
        if (sameEpisodes) {
            for (std::size_t index = 0; index < rows.size(); ++index) {
                rows[index]->setWatchState(episodes[index]);
            }
        } else {
            for (MediaEpisodeRow *row : rows) {
                delete row;
            }
            rows.clear();
            for (const MediaEpisode &episode : episodes) {
                auto *row = new MediaEpisodeRow(apiClient, media, season, episode, this);
                episodeLayout->addWidget(row);
                rows.push_back(row);
            }
        }
        episodeNotice->setText("Keine Episoden vorhanden.");
        episodeNotice->setVisible(episodes.empty());
    }

    MediaSeason season;
    QToolButton *toggle;
    QLabel *episodeNotice;
    int episodeRequestId = 0;
    bool episodesLoaded = false;

private:
    QLabel *count;
    QVBoxLayout *episodeLayout;
    std::vector<MediaEpisodeRow *> rows;
};

MediaSeriesView::MediaSeriesView(ApiClient &apiClient, QWidget *parent)
    : QWidget(parent), apiClient(apiClient) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    auto *title = mediaText("Serienfortschritt", this);
    title->setObjectName("mediaSectionTitle");
    layout->addWidget(title);
    progressNotice = mediaText({}, this);
    progressNotice->setObjectName("mediaMutedText");
    layout->addWidget(progressNotice);
    regularLabel = mediaText({}, this);
    regularProgress = progressBar(this);
    regularProgress->setMaximumWidth(360);
    specialsLabel = mediaText({}, this);
    specialsLabel->setObjectName("mediaMutedText");
    layout->addWidget(regularLabel);
    auto *progressRow = new QHBoxLayout();
    progressRow->addWidget(regularProgress, 1);
    progressRow->addStretch(1);
    layout->addLayout(progressRow);
    layout->addWidget(specialsLabel);

    auto *seasonsTitle = mediaText("Staffeln und Episoden", this);
    seasonsTitle->setObjectName("mediaSectionTitle");
    layout->addSpacing(10);
    layout->addWidget(seasonsTitle);
    seasonNotice = mediaText({}, this);
    seasonNotice->setObjectName("mediaMutedText");
    layout->addWidget(seasonNotice);
    seasonLayout = new QVBoxLayout();
    seasonLayout->setSpacing(8);
    layout->addLayout(seasonLayout);

    connect(&apiClient, &ApiClient::mediaSeriesProgressReceived, this,
            [this](const int id, const MediaSeriesProgress &progress) {
                if (id == progressRequestId) {
                    progressRequestId = 0;
                    showProgress(progress);
                }
            });
    connect(&apiClient, &ApiClient::mediaSeasonsReceived, this,
            [this](const int id, const std::vector<MediaSeason> &seasons) {
                if (id == seasonsRequestId) {
                    seasonsRequestId = 0;
                    showSeasons(seasons);
                }
            });
    connect(&apiClient, &ApiClient::mediaEpisodesReceived, this,
            [this](const int id, const std::vector<MediaEpisode> &episodes) {
                for (MediaSeasonCard *card : cards) {
                    if (id == card->episodeRequestId) {
                        card->episodeRequestId = 0;
                        card->showEpisodes(this->apiClient, series, episodes);
                        break;
                    }
                }
            });
    connect(&apiClient, &ApiClient::mediaEpisodeWatchedUpdateFinished, this,
            [this](int, const int entityId, const int seasonId, const bool successful) {
                if (!successful || entityId != series.entityId) {
                    return;
                }
                loadProgress();
                for (MediaSeasonCard *card : cards) {
                    if (card->season.id == seasonId && (card->episodesLoaded || card->toggle->isChecked())) {
                        loadEpisodes(card);
                        break;
                    }
                }
            });
    connect(&apiClient, &ApiClient::mediaDetailsLoadFailed, this, [this](const int id) {
        if (id == progressRequestId) {
            progressRequestId = 0;
            showProgress({});
            progressNotice->setText("Der Serienfortschritt konnte nicht geladen werden.");
        } else if (id == seasonsRequestId) {
            seasonsRequestId = 0;
            seasonNotice->setText("Staffeln konnten nicht geladen werden.");
            seasonNotice->show();
        } else {
            for (MediaSeasonCard *card : cards) {
                if (id == card->episodeRequestId) {
                    card->episodeRequestId = 0;
                    card->episodeNotice->setText("Episoden konnten nicht geladen werden.");
                    card->episodeNotice->show();
                    break;
                }
            }
        }
    });
    hide();
}

void MediaSeriesView::setMedia(const Media &media) {
    series = media;
    seasonsRequestId = 0;
    progressRequestId = 0;
    for (MediaSeasonCard *card : cards) {
        delete card;
    }
    cards.clear();
    setVisible(series.mediaType == "tv");
    if (series.mediaType != "tv") {
        return;
    }
    showProgress({});
    progressNotice->setText("Serienfortschritt wird geladen …");
    seasonNotice->setText("Staffeln werden geladen …");
    seasonNotice->show();
    loadProgress();
}

void MediaSeriesView::loadProgress() {
    progressRequestId = apiClient.getMediaSeriesProgress(series.entityId);
    seasonsRequestId = apiClient.getMediaSeasons(series.entityId);
}

void MediaSeriesView::loadEpisodes(MediaSeasonCard *card) {
    card->episodeRequestId = apiClient.getMediaEpisodes(series.entityId, card->season.id);
    card->episodeNotice->setText("Episoden werden geladen …");
    card->episodeNotice->setVisible(!card->episodesLoaded);
}

void MediaSeriesView::showSeasons(const std::vector<MediaSeason> &seasons) {
    seasonNotice->setText("Keine Staffeln vorhanden.");
    seasonNotice->setVisible(seasons.empty());
    if (!cards.empty()) {
        for (MediaSeasonCard *card : cards) {
            const auto updated = std::find_if(seasons.begin(), seasons.end(), [card](const MediaSeason &season) {
                return season.id == card->season.id;
            });
            if (updated != seasons.end()) {
                card->setCounts(*updated);
            }
        }
        return;
    }

    auto sorted = seasons;
    std::stable_partition(sorted.begin(), sorted.end(), [](const MediaSeason &season) {
        return season.number != 0;
    });
    MediaSeasonCard *firstUnfinished = nullptr;
    for (const MediaSeason &season : sorted) {
        auto *card = new MediaSeasonCard(series, season, this);
        seasonLayout->addWidget(card);
        cards.push_back(card);
        connect(card->toggle, &QToolButton::toggled, this, [this, card](const bool open) {
            if (open && !card->episodesLoaded && card->episodeRequestId == 0) {
                loadEpisodes(card);
            }
        });
        if (!firstUnfinished && season.number != 0 && season.episodeCount > season.watchedEpisodeCount) {
            firstUnfinished = card;
        }
    }
    if (firstUnfinished) {
        firstUnfinished->toggle->setChecked(true);
    }
}

void MediaSeriesView::showProgress(const MediaSeriesProgress &progress) {
    regularLabel->setVisible(progress.regular.episodeCount > 0);
    regularProgress->setVisible(progress.regular.episodeCount > 0);
    regularLabel->setText(QString("%1 von %2 regulären Episoden gesehen")
        .arg(progress.regular.watchedEpisodeCount).arg(progress.regular.episodeCount));
    setProgressBar(*regularProgress, progress.regular.percentage);
    specialsLabel->setVisible(progress.specials.episodeCount > 0);
    specialsLabel->setText(QString("Sonderfolgen · %1 von %2 gesehen")
        .arg(progress.specials.watchedEpisodeCount).arg(progress.specials.episodeCount));
    progressNotice->setText("Keine Episoden für die Fortschrittsanzeige vorhanden.");
    progressNotice->setVisible(progress.regular.episodeCount == 0 && progress.specials.episodeCount == 0);
}
