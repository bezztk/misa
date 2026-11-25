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
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(7);
        auto *heading = new QHBoxLayout();
        heading->setSpacing(12);
        auto *picture = new MediaImage(this);
        picture->setFixedSize(128, 72);
        picture->setFocusPolicy(Qt::NoFocus);
        picture->setAttribute(Qt::WA_TransparentForMouseEvents);
        const QString imageName = QString("seasons/%1/episodes/%2/still").arg(season.number).arg(episode.number);
        picture->setFile(mediaImageFile(mediaImagePath(media, imageName, episode.stillPath)),
                         QSize(256, 144));
        heading->addWidget(picture, 0, Qt::AlignTop);

        auto *text = new QVBoxLayout();
        text->setSpacing(5);
        QString title = QString("E%1").arg(episode.number, 2, 10, QChar('0'));
        if (!episode.title.isEmpty()) {
            title += " · " + episode.title;
        }
        auto *titleLabel = mediaText(title, this);
        titleLabel->setObjectName("mediaEpisodeTitle");
        text->addWidget(titleLabel);
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
        heading->addLayout(text, 1);
        layout->addLayout(heading);

        auto *description = new MediaDescription(260, this);
        description->setText(episode.description);
        layout->addWidget(description);

        auto *controls = new QHBoxLayout();
        controls->setSpacing(8);
        status = new QComboBox(this);
        status->setAccessibleName("Sichtungsstatus der Episode");
        addMediaWatchStatuses(*status);
        markWatched = new QPushButton("Als gesehen markieren", this);
        markWatched->setCursor(Qt::PointingHandCursor);
        watchedAt = mediaText({}, this);
        watchedAt->setObjectName("mediaWatchedAt");
        controls->addWidget(status);
        controls->addStretch(1);
        layout->addLayout(controls);
        layout->addWidget(markWatched, 0, Qt::AlignLeft);
        layout->addWidget(watchedAt);
        setWatchState(episode);

        const auto chooseStatus = [this, &apiClient, mediaId = media.entityId,
                                   seasonId = season.id](const int selectedStatus) {
            if (selectedStatus == savedStatus) {
                return;
            }
            status->setCurrentIndex(status->findData(selectedStatus));
            requestId = apiClient.updateMediaEpisodeStatus(mediaId, seasonId, episodeId, selectedStatus);
            status->setEnabled(false);
            markWatched->setEnabled(false);
        };
        connect(status, &QComboBox::activated, this, [this, chooseStatus](const int index) {
            chooseStatus(status->itemData(index).toInt());
        });
        connect(markWatched, &QPushButton::clicked, this, [chooseStatus] { chooseStatus(3); });
        connect(&apiClient, &ApiClient::mediaEpisodeStatusUpdateFinished, this,
                [this](const int id, int, int, const bool successful) {
                    if (id != requestId) {
                        return;
                    }
                    requestId = 0;
                    if (successful) {
                        savedStatus = status->currentData().toInt();
                        watchedAt->hide();
                    } else {
                        status->setCurrentIndex(status->findData(savedStatus));
                    }
                    status->setEnabled(true);
                    markWatched->setVisible(savedStatus != 3);
                    markWatched->setEnabled(true);
                });
    }

    int id() const { return episodeId; }

    void setWatchState(const MediaEpisode &episode) {
        if (requestId != 0) {
            return;
        }
        savedStatus = episode.watchStatusId;
        status->setCurrentIndex(status->findData(savedStatus));
        markWatched->setVisible(savedStatus != 3);
        watchedAt->setVisible(episode.watchedAt.isValid());
        watchedAt->setText(QString("Abgeschlossen am %1")
            .arg(episode.watchedAt.toLocalTime().toString("dd.MM.yyyy · HH:mm")));
    }

private:
    int episodeId;
    int savedStatus = 0;
    int requestId = 0;
    QComboBox *status;
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

        auto *text = new QVBoxLayout();
        text->setSpacing(5);
        toggle = new QToolButton(this);
        toggle->setObjectName("mediaSeasonToggle");
        toggle->setText(season.number == 0 ? QStringLiteral("Sonderfolgen")
                                        : QString("Staffel %1").arg(season.number));
        toggle->setCheckable(true);
        toggle->setArrowType(Qt::RightArrow);
        toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toggle->setCursor(Qt::PointingHandCursor);
        text->addWidget(toggle, 0, Qt::AlignLeft);
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
        bar = progressBar(this);
        text->addWidget(count);
        text->addWidget(bar);
        header->addLayout(text, 1);
        layout->addLayout(header);

        auto *description = new MediaDescription(300, this);
        description->setText(season.description);
        layout->addWidget(description);

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
        season.completedEpisodeCount = updated.completedEpisodeCount;
        count->setText(QString("%1 von %2 Episoden abgeschlossen")
            .arg(season.completedEpisodeCount).arg(season.episodeCount));
        bar->setVisible(season.episodeCount > 0);
        if (season.episodeCount > 0) {
            setProgressBar(*bar, 100.0 * season.completedEpisodeCount / season.episodeCount);
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
    QProgressBar *bar;
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
    specialsLabel = mediaText({}, this);
    specialsProgress = progressBar(this);
    layout->addWidget(regularLabel);
    layout->addWidget(regularProgress);
    layout->addWidget(specialsLabel);
    layout->addWidget(specialsProgress);

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
    connect(&apiClient, &ApiClient::mediaEpisodeStatusUpdateFinished, this,
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
        if (!firstUnfinished && season.number != 0 && season.episodeCount > season.completedEpisodeCount) {
            firstUnfinished = card;
        }
    }
    if (firstUnfinished) {
        firstUnfinished->toggle->setChecked(true);
    }
}

void MediaSeriesView::showProgress(const MediaSeriesProgress &progress) {
    const auto show = [](const MediaEpisodeProgress &value, const QString &text,
                         QLabel *label, QProgressBar *bar) {
        const bool available = value.episodeCount > 0;
        label->setVisible(available);
        bar->setVisible(available);
        if (available) {
            label->setText(text.arg(value.completedEpisodeCount).arg(value.episodeCount));
            setProgressBar(*bar, value.percentage);
        }
    };
    show(progress.regular, "%1 von %2 regulären Episoden abgeschlossen", regularLabel, regularProgress);
    show(progress.specials, "%1 von %2 Sonderfolgen abgeschlossen", specialsLabel, specialsProgress);
    progressNotice->setText("Keine Episoden für die Fortschrittsanzeige vorhanden.");
    progressNotice->setVisible(progress.regular.episodeCount == 0 && progress.specials.episodeCount == 0);
}
