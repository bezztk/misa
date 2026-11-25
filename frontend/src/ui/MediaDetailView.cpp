#include "MediaDetailView.h"

#include "../api/ApiClient.h"
#include "MediaImages.h"
#include "MediaSeriesView.h"
#include "MediaUi.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLinearGradient>
#include <QLocale>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QStyle>
#include <QUrl>

#include <limits>
#include <utility>

namespace {
const QColor BACKGROUND(30, 30, 30);
constexpr int FADE_HEIGHT = 80;
constexpr int KEYWORD_PREVIEW = 12;

constexpr auto DETAIL_STYLE = R"(
    QWidget#mediaDetailContent { background: #1E1E1E; }
    QLabel#mediaDetailTitle { color: #F5F5F5; font-size: 34px; font-weight: 600; }
    QLabel#mediaSectionTitle { color: #E5E5E5; font-size: 18px; font-weight: 600; }
    QLabel#mediaEpisodeTitle { color: #E5E5E5; font-weight: 600; }
    QLabel#mediaMutedText { color: #B8B8B8; }
    QLabel#mediaTmdbRating { color: #E1BC84; font-weight: 600; }
    QLabel#mediaWatchedAt { color: #9DCCAE; }
    QFrame#mediaSeason { background: #262626; border: 1px solid #383838; border-radius: 4px; }
    QFrame#mediaEpisode { background: #222222; border-radius: 4px; }
    QWidget#mediaDetailContent QComboBox, QWidget#mediaDetailContent QSpinBox,
    QWidget#mediaDetailContent QPlainTextEdit, QWidget#mediaDetailContent QPushButton,
    QToolButton#mediaDetailBack, QToolButton#mediaSeasonToggle, QToolButton#mediaTechnicalToggle {
        background: #262626;
        color: #E5E5E5;
        border: 1px solid #454545;
        border-radius: 4px;
        padding: 5px 8px;
    }
    QWidget#mediaDetailContent QPushButton:hover, QToolButton#mediaDetailBack:hover,
    QToolButton#mediaSeasonToggle:hover, QToolButton#mediaTechnicalToggle:hover { background: #383838; }
    QWidget#mediaDetailContent QPushButton:disabled,
    QWidget#mediaDetailContent QComboBox:disabled { color: #888888; }
    QProgressBar {
        background: #1E1E1E;
        color: #E5E5E5;
        border: 1px solid #414141;
        border-radius: 3px;
        min-height: 15px;
        text-align: center;
    }
    QProgressBar::chunk { background: #536D8D; border-radius: 2px; }
)";

QLabel *sectionTitle(const QString &text, QWidget *parent) {
    auto *label = mediaText(text, parent);
    label->setObjectName("mediaSectionTitle");
    return label;
}

QString productionLabel(const QString &status) {
    const std::pair<const char *, const char *> labels[]{
        {"Released", "Veröffentlicht"}, {"Returning Series", "Laufend"}, {"Ended", "Beendet"},
        {"Canceled", "Eingestellt"}, {"In Production", "In Produktion"}, {"Planned", "Geplant"},
        {"Post Production", "In Nachbearbeitung"}, {"Rumored", "Gerücht"}, {"Pilot", "Pilotfolge"}
    };
    for (const auto &[key, label] : labels) {
        if (status == QString::fromLatin1(key)) {
            return QString::fromUtf8(label);
        }
    }
    return status;
}

QString tmdbRatingText(const Media &media) {
    const QLocale locale(QLocale::German);
    QStringList summary;
    if (media.tmdbVoteAverage && media.tmdbVoteCount != 0) {
        summary.append(QString("%1/10").arg(locale.toString(*media.tmdbVoteAverage, 'f', 1)));
    }
    if (media.tmdbVoteCount) {
        summary.append(*media.tmdbVoteCount == 1 ? QStringLiteral("1 Stimme")
            : QString("%1 Stimmen").arg(locale.toString(*media.tmdbVoteCount)));
    }
    return summary.isEmpty() ? QString() : QString("TMDB · %1").arg(summary.join(" · "));
}

bool isTrailerUrl(const QUrl &url) {
    return url.scheme() == "https" && url.host() == "www.youtube.com" && url.path() == "/watch";
}
}

class MediaDetailHero final : public QWidget {
public:
    explicit MediaDetailHero(QWidget *parent) : QWidget(parent) {
        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        policy.setHeightForWidth(true);
        setSizePolicy(policy);
    }

    void setBackdrop(const QPixmap &image) {
        backdrop = image;
        updateGeometry();
        update();
    }

    int heightForWidth(const int width) const override {
        const int contentHeight = qMax(minimumSizeHint().height(), QWidget::heightForWidth(width));
        return qMax(backdropHeight(width), contentHeight);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.fillRect(rect(), BACKGROUND);
        const int imageHeight = backdrop.isNull() ? height() : backdropHeight(width());
        const QRect bounds(0, 0, width(), imageHeight);
        if (backdrop.isNull()) {
            QLinearGradient background(0, 0, 0, imageHeight);
            background.setColorAt(0, QColor("#303030"));
            background.setColorAt(1, BACKGROUND);
            painter.fillRect(bounds, background);
            return;
        }
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawPixmap(bounds, backdrop);

        QLinearGradient shade(0, 0, width(), 0);
        shade.setColorAt(0, QColor(0, 0, 0, 48));
        shade.setColorAt(1, QColor(0, 0, 0, 190));
        painter.fillRect(bounds, shade);
        const int fadeStart = qMax(0, imageHeight - FADE_HEIGHT);
        QLinearGradient fade(0, fadeStart, 0, imageHeight);
        fade.setColorAt(0, QColor(30, 30, 30, 0));
        fade.setColorAt(1, BACKGROUND);
        painter.fillRect(QRect(0, fadeStart, width(), imageHeight - fadeStart), fade);
    }

private:
    int backdropHeight(const int width) const {
        return backdrop.isNull() ? 0 : qRound(qreal(width) * backdrop.height() / backdrop.width());
    }

    QPixmap backdrop;
};

MediaDetailView::MediaDetailView(ApiClient &apiClient, QWidget *parent) : QScrollArea(parent) {
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setStyleSheet(DETAIL_STYLE);

    auto *content = new QWidget();
    content->setObjectName("mediaDetailContent");
    content->setAttribute(Qt::WA_StyledBackground);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    hero = new MediaDetailHero(content);
    auto *heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(20, 16, 20, 18);
    heroLayout->setSpacing(12);
    auto *back = new QToolButton(hero);
    back->setObjectName("mediaDetailBack");
    back->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    back->setText("Zurück zur Liste");
    back->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    back->setCursor(Qt::PointingHandCursor);
    heroLayout->addWidget(back, 0, Qt::AlignLeft);
    heroLayout->addStretch(1);
    connect(back, &QToolButton::clicked, this, &MediaDetailView::backRequested);

    heading = new QHBoxLayout();
    heading->setSpacing(18);
    cover = new MediaImage(hero, Qt::KeepAspectRatio);
    cover->setAccessibleName("Poster vergrößern");
    cover->setCursor(Qt::PointingHandCursor);
    heading->addWidget(cover, 0, Qt::AlignLeft | Qt::AlignBottom);
    connect(cover, &MediaImage::clicked, this, [this] {
        showMediaImages(this, {mediaImageFile(mediaImagePath(media, "poster", media.posterPath))});
    });

    auto *intro = new QVBoxLayout();
    intro->setSpacing(6);
    intro->addStretch(1);
    mediaTypeLabel = mediaText({}, hero);
    mediaTypeLabel->setObjectName("mediaMutedText");
    intro->addWidget(mediaTypeLabel);
    title = mediaText({}, hero);
    title->setObjectName("mediaDetailTitle");
    intro->addWidget(title);
    originalTitle = mediaText({}, hero);
    originalTitle->setObjectName("mediaMutedText");
    intro->addWidget(originalTitle);
    facts = mediaText({}, hero);
    facts->setObjectName("mediaMutedText");
    intro->addWidget(facts);
    genres = mediaText({}, hero);
    genres->setObjectName("mediaMutedText");
    intro->addWidget(genres);
    description = new MediaDescription(420, hero);
    intro->addWidget(description);
    tmdbRating = mediaText({}, hero);
    tmdbRating->setObjectName("mediaTmdbRating");
    intro->addWidget(tmdbRating);

    actions = new QHBoxLayout();
    actions->setSpacing(10);
    auto *statusField = new QVBoxLayout();
    statusField->setSpacing(4);
    auto *statusLabel = new QLabel("&Sichtungsstatus", hero);
    watchStatus = new QComboBox(hero);
    addMediaWatchStatuses(*watchStatus);
    statusLabel->setBuddy(watchStatus);
    statusField->addWidget(statusLabel);
    statusField->addWidget(watchStatus, 0, Qt::AlignLeft);
    actions->addLayout(statusField);
    auto *ratingField = new QVBoxLayout();
    ratingField->setSpacing(4);
    auto *ratingLabel = new QLabel("&Persönliche Bewertung", hero);
    rating = new QComboBox(hero);
    ratingLabel->setBuddy(rating);
    ratingField->addWidget(ratingLabel);
    ratingField->addWidget(rating, 0, Qt::AlignLeft);
    actions->addLayout(ratingField);
    trailer = new QPushButton("Trailer ansehen", hero);
    trailer->setCursor(Qt::PointingHandCursor);
    actions->addWidget(trailer, 0, Qt::AlignBottom | Qt::AlignLeft);
    actions->addStretch(1);
    intro->addLayout(actions);
    heading->addLayout(intro, 1);
    heroLayout->addLayout(heading);
    layout->addWidget(hero);

    auto *body = new QVBoxLayout();
    body->setContentsMargins(20, 0, 20, 28);
    body->setSpacing(12);
    series = new MediaSeriesView(apiClient, content);
    body->addWidget(series);

    body->addWidget(sectionTitle("Persönliche Informationen", content));
    auto *personalFields = new QFormLayout();
    personalFields->setRowWrapPolicy(QFormLayout::WrapLongRows);
    watchCount = new QSpinBox(content);
    watchCount->setRange(0, std::numeric_limits<int>::max());
    personalFields->addRow("Sichtungsanzahl", watchCount);
    completionDate = mediaText({}, content);
    completionDate->setToolTip("Wird durch den Sichtungsstatus gesetzt.");
    personalFields->addRow("Abschlussdatum", completionDate);
    body->addLayout(personalFields);
    auto *notesLabel = new QLabel("Persönliche &Notizen", content);
    notes = new QPlainTextEdit(content);
    notes->setFixedHeight(98);
    notesLabel->setBuddy(notes);
    body->addWidget(notesLabel);
    body->addWidget(notes);
    savePersonal = new QPushButton("Speichern", content);
    body->addWidget(savePersonal, 0, Qt::AlignLeft);

    gallerySection = new QWidget(content);
    auto *galleryLayout = new QVBoxLayout(gallerySection);
    galleryLayout->setContentsMargins(0, 0, 0, 0);
    galleryLayout->addWidget(sectionTitle("Bildergalerie", gallerySection));
    gallery = new MediaGallery(gallerySection);
    galleryLayout->addWidget(gallery);
    body->addWidget(gallerySection);

    releaseDate = mediaText({}, content);
    releaseDate->setObjectName("mediaMutedText");
    body->addWidget(releaseDate);
    keywordSection = new QWidget(content);
    auto *keywordLayout = new QVBoxLayout(keywordSection);
    keywordLayout->setContentsMargins(0, 0, 0, 0);
    keywordLayout->addWidget(sectionTitle("Keywords", keywordSection));
    keywords = mediaText({}, keywordSection);
    keywords->setTextFormat(Qt::RichText);
    keywordLayout->addWidget(keywords);
    moreKeywords = new QToolButton(keywordSection);
    moreKeywords->setAutoRaise(true);
    moreKeywords->setCheckable(true);
    moreKeywords->setCursor(Qt::PointingHandCursor);
    keywordLayout->addWidget(moreKeywords, 0, Qt::AlignLeft);
    body->addWidget(keywordSection);

    technicalToggle = new QToolButton(content);
    technicalToggle->setObjectName("mediaTechnicalToggle");
    technicalToggle->setText("Technische Informationen");
    technicalToggle->setCheckable(true);
    technicalToggle->setArrowType(Qt::RightArrow);
    technicalToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    technicalToggle->setCursor(Qt::PointingHandCursor);
    body->addWidget(technicalToggle, 0, Qt::AlignLeft);
    auto *technical = new QWidget(content);
    technicalDetails = new QFormLayout(technical);
    technicalDetails->setContentsMargins(0, 0, 0, 0);
    technicalDetails->setRowWrapPolicy(QFormLayout::WrapLongRows);
    technicalDetails->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    body->addWidget(technical);
    technical->hide();
    connect(technicalToggle, &QToolButton::toggled, this, [this, technical](const bool open) {
        technicalToggle->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
        technical->setVisible(open);
    });
    connect(moreKeywords, &QToolButton::toggled, this, &MediaDetailView::updateKeywords);
    connect(trailer, &QPushButton::clicked, this, [this] {
        const QUrl url(media.trailerUrl);
        if (isTrailerUrl(url)) {
            QDesktopServices::openUrl(url);
        }
    });

    const auto updateRatingOptions = [this, &apiClient] {
        rating->clear();
        rating->addItem("Nicht bewertet");
        const auto &options = apiClient.metadata().mediaRatings;
        for (const MetadataItem &option : options) {
            rating->addItem(option.displayName.value(), option.id);
        }
        setRating(savedRating);
        rating->setEnabled(!options.empty() && ratingUpdateEntityId == 0);
    };
    connect(&apiClient, &ApiClient::metadataReceived, this, updateRatingOptions);
    updateRatingOptions();
    connect(rating, &QComboBox::activated, this, [this, &apiClient](const int index) {
        const QVariant data = rating->itemData(index);
        const auto value = data.isValid()
            ? std::optional(static_cast<MediaRating>(data.toInt()))
            : std::nullopt;
        if (value != savedRating) {
            ratingUpdateEntityId = media.entityId;
            rating->setEnabled(false);
            apiClient.updateMediaRating(media.entityId, value);
        }
    });
    connect(&apiClient, &ApiClient::mediaRatingUpdateFinished, this,
            [this](const int id, const std::optional<MediaRating> value, const bool successful) {
                if (id == ratingUpdateEntityId) {
                    ratingUpdateEntityId = 0;
                    if (id == media.entityId) {
                        setRating(successful ? value : savedRating);
                    }
                    rating->setEnabled(true);
                }
            });
    connect(watchStatus, &QComboBox::activated, this, [this, &apiClient](const int index) {
        const int status = watchStatus->itemData(index).toInt();
        if (status != media.watchStatusId) {
            statusEntityId = media.entityId;
            pendingWatchStatus = status;
            statusRequestId = apiClient.updateMediaPersonalData(media.entityId, {{"watch_status_id", status}});
            watchStatus->setEnabled(false);
        }
    });
    connect(watchCount, &QSpinBox::valueChanged, this, &MediaDetailView::updatePersonalSaveButton);
    connect(notes, &QPlainTextEdit::textChanged, this, &MediaDetailView::updatePersonalSaveButton);
    connect(savePersonal, &QPushButton::clicked, this, [this, &apiClient] {
        personalEntityId = media.entityId;
        pendingWatchCount = watchCount->value();
        pendingNotes = notes->toPlainText();
        const QJsonValue value = pendingNotes.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(pendingNotes);
        personalRequestId = apiClient.updateMediaPersonalData(media.entityId, {
            {"watch_count", pendingWatchCount}, {"personal_notes", value}
        });
        watchCount->setEnabled(false);
        notes->setEnabled(false);
        updatePersonalSaveButton();
    });
    connect(&apiClient, &ApiClient::mediaPersonalDataUpdateFinished, this,
            [this](const int id, const bool successful) {
                if (id == statusRequestId) {
                    statusRequestId = 0;
                    if (media.entityId == statusEntityId) {
                        if (successful) {
                            media.watchStatusId = pendingWatchStatus;
                        }
                        watchStatus->setCurrentIndex(watchStatus->findData(media.watchStatusId));
                    }
                    watchStatus->setEnabled(true);
                } else if (id == personalRequestId) {
                    personalRequestId = 0;
                    if (successful && media.entityId == personalEntityId) {
                        media.watchCount = pendingWatchCount;
                        media.personalNotes = pendingNotes;
                    }
                    watchCount->setEnabled(true);
                    notes->setEnabled(true);
                    updatePersonalSaveButton();
                }
            });

    layout->addLayout(body);
    layout->addStretch(1);
    setWidget(content);
    updateHeaderLayout();
}

void MediaDetailView::setMedia(const Media &entry) {
    media = entry;
    setRating(media.personalRating);
    const bool isSeries = media.mediaType == "tv";
    mediaTypeLabel->setText(isSeries ? "Serie" : "Film");
    title->setText(media.displayTitle());
    originalTitle->setText(media.originalTitle);
    originalTitle->setVisible(!media.originalTitle.isEmpty() && media.originalTitle != title->text());

    QStringList info;
    if (media.releaseDate.isValid()) {
        info.append(QString::number(media.releaseDate.year()));
    }
    if (isSeries) {
        info.append(media.seasonCount == 1 ? QStringLiteral("1 Staffel")
                                                : QString("%1 Staffeln").arg(media.seasonCount));
        info.append(media.episodeCount == 1 ? QStringLiteral("1 Episode")
                                                 : QString("%1 Episoden inkl. Sonderfolgen").arg(media.episodeCount));
    } else if (media.runtimeMinutes) {
        info.append(QString("%1 Min.").arg(*media.runtimeMinutes));
    }
    if (!media.productionStatus.isEmpty()) {
        info.append(productionLabel(media.productionStatus));
    }
    facts->setText(info.join(" · "));
    facts->setVisible(!info.isEmpty());
    genres->setText(media.genres.join(" · "));
    genres->setVisible(!media.genres.isEmpty());
    description->setText(media.description);
    tmdbRating->setText(tmdbRatingText(media));
    tmdbRating->setVisible(!tmdbRating->text().isEmpty());
    trailer->setVisible(isTrailerUrl(QUrl(media.trailerUrl)));

    const QString posterFile = mediaImageFile(mediaImagePath(media, "poster", media.posterPath));
    cover->setEnabled(QFileInfo(posterFile).isFile());
    cover->setFile(posterFile, QSize(480, 720));
    const QString backdropFile = mediaImageFile(mediaImagePath(media, "backdrop", media.backdropPath));
    const int imageRequest = ++backdropRequestId;
    hero->setBackdrop({});
    loadMediaPixmap(*this, backdropFile, QSize(1440, 900), [this, imageRequest](const QPixmap &image) {
        if (imageRequest == backdropRequestId) {
            hero->setBackdrop(image);
        }
    }, true);

    const bool statusPending = statusRequestId != 0 && media.entityId == statusEntityId;
    watchStatus->setEnabled(statusRequestId == 0);
    watchStatus->setCurrentIndex(watchStatus->findData(statusPending ? pendingWatchStatus : media.watchStatusId));
    const bool personalPending = personalRequestId != 0 && media.entityId == personalEntityId;
    watchCount->setEnabled(personalRequestId == 0);
    notes->setEnabled(personalRequestId == 0);
    watchCount->setValue(personalPending ? pendingWatchCount : media.watchCount);
    notes->setPlainText(personalPending ? pendingNotes : media.personalNotes);
    updatePersonalSaveButton();
    updateCompletionDate(media.entityId, media.completedAt);

    QStringList galleryFiles;
    if (QFileInfo(backdropFile).isFile()) {
        galleryFiles.append(backdropFile);
    }
    for (int index = 0; index < media.additionalBackdropPaths.size(); ++index) {
        const QString file = mediaImageFile(mediaImagePath(media, QString("backdrops/%1").arg(index + 1),
                                                          media.additionalBackdropPaths[index]));
        if (QFileInfo(file).isFile()) {
            galleryFiles.append(file);
        }
    }
    gallery->setFiles(galleryFiles);
    gallerySection->setVisible(!galleryFiles.isEmpty());
    releaseDate->setText(media.releaseDate.isValid()
        ? QString("%1 · %2").arg(isSeries ? QStringLiteral("Serienstart") : QStringLiteral("Veröffentlichung"),
                                 media.releaseDate.toString("dd.MM.yyyy"))
        : QString());
    releaseDate->setVisible(media.releaseDate.isValid());
    keywordSection->setVisible(!media.keywords.isEmpty());
    moreKeywords->setChecked(false);
    updateKeywords();
    technicalToggle->setChecked(false);
    setTechnicalDetails();
    series->setMedia(media);
    verticalScrollBar()->setValue(0);
}

void MediaDetailView::resizeEvent(QResizeEvent *event) {
    QScrollArea::resizeEvent(event);
    updateHeaderLayout();
}

void MediaDetailView::updateHeaderLayout() {
    if (!heading) {
        return;
    }
    const int width = viewport()->width();
    heading->setDirection(width < 620 ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    const int posterWidth = width >= 950 ? 200 : width >= 620 ? 156 : 112;
    cover->setFixedSize(posterWidth, posterWidth * 3 / 2);
    actions->setDirection(width < 740 ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
}

void MediaDetailView::setRating(const std::optional<MediaRating> value) {
    savedRating = value;
    rating->setCurrentIndex(value ? rating->findData(static_cast<int>(*value)) : 0);
}

void MediaDetailView::updateCompletionDate(const int entityId, const QDateTime &completedAt) {
    if (media.entityId == entityId) {
        completionDate->setText(completedAt.isValid()
            ? completedAt.toLocalTime().toString("dd.MM.yyyy · HH:mm") : QStringLiteral("–"));
    }
}

void MediaDetailView::updatePersonalSaveButton() {
    savePersonal->setEnabled(personalRequestId == 0
        && (watchCount->value() != media.watchCount || notes->toPlainText() != media.personalNotes));
}

void MediaDetailView::updateKeywords() {
    const bool showAll = moreKeywords->isChecked();
    const qsizetype count = showAll ? media.keywords.size()
        : qMin<qsizetype>(media.keywords.size(), KEYWORD_PREVIEW);
    QStringList tags;
    for (qsizetype index = 0; index < count; ++index) {
        tags.append(QString("<span style=\"background-color:#303030; color:#e5e5e5\">&nbsp;%1&nbsp;</span>")
            .arg(media.keywords[index].toHtmlEscaped()));
    }
    keywords->setText(tags.join(" "));
    moreKeywords->setVisible(media.keywords.size() > KEYWORD_PREVIEW);
    moreKeywords->setText(showAll ? "Weniger Keywords anzeigen"
        : QString("Alle %1 Keywords anzeigen").arg(media.keywords.size()));
}

void MediaDetailView::setTechnicalDetails() {
    while (technicalDetails->rowCount() > 0) {
        technicalDetails->removeRow(0);
    }
    const auto add = [this](const QString &caption, const QString &value, const bool link = false) {
        if (value.isEmpty()) {
            return;
        }
        auto *label = mediaText(value, technicalDetails->parentWidget());
        label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
        if (link && QUrl(value).scheme() == "https") {
            label->setTextFormat(Qt::RichText);
            label->setText(QString("<a href=\"%1\">%1</a>").arg(value.toHtmlEscaped()));
            label->setOpenExternalLinks(true);
        }
        technicalDetails->addRow(caption, label);
    };
    add("Entity-ID", QString::number(media.entityId));
    add("TMDB-ID", QString::number(media.tmdbId));
    add("Zuletzt synchronisiert", media.lastSyncedAt.toLocalTime().toString("dd.MM.yyyy · HH:mm"));
    add("Lokaler Posterpfad", mediaImagePath(media, "poster", media.posterPath));
    add("Lokaler Backdrop-Pfad", mediaImagePath(media, "backdrop", media.backdropPath));
    for (int index = 0; index < media.additionalBackdropPaths.size(); ++index) {
        add(QString("Galeriebild %1").arg(index + 1),
            mediaImagePath(media, QString("backdrops/%1").arg(index + 1), media.additionalBackdropPaths[index]));
    }
    add("TMDB-Poster", media.posterUrl, true);
    add("TMDB-Backdrop", media.backdropUrl, true);
    add("Trailer-URL", media.trailerUrl, true);
}
