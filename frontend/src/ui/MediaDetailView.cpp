#include "MediaDetailView.h"

#include "../api/ApiClient.h"
#include "DialogUtils.h"
#include "MediaImages.h"
#include "MediaSeriesView.h"
#include "MediaUi.h"

#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QJsonObject>
#include <QLinearGradient>
#include <QLocale>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QUrl>

#include <utility>

namespace {
const QColor BACKGROUND(30, 30, 30);
constexpr int FADE_HEIGHT = 90;
constexpr int KEYWORD_PREVIEW = 12;

constexpr auto DETAIL_STYLE = R"(
    QWidget#mediaDetailContent { background: #1E1E1E; }
    QLabel#mediaDetailTitle { color: #F5F5F5; font-size: 34px; font-weight: 600; }
    QLabel#mediaSectionTitle { color: #E5E5E5; font-size: 18px; font-weight: 600; }
    QLabel#mediaSeasonTitle { color: #E5E5E5; font-size: 15px; font-weight: 600; }
    QLabel#mediaEpisodeTitle { color: #E5E5E5; font-weight: 600; }
    QLabel#mediaMutedText { color: #B8B8B8; }
    QLabel#mediaTmdbRating { color: #E1BC84; font-weight: 600; }
    QLabel#mediaWatchedAt { color: #9DCCAE; }
    QLabel#mediaKeyword {
        background: #303030;
        color: #E5E5E5;
        border-radius: 4px;
        padding: 3px 8px;
    }
    QFrame#mediaSeason { background: #262626; border: 1px solid #383838; border-radius: 4px; }
    QFrame#mediaEpisode { background: #222222; border-radius: 4px; }
    QWidget#mediaDetailContent QComboBox, QWidget#mediaDetailContent QPushButton,
    QToolButton#mediaSeasonToggle, QToolButton#mediaTechnicalToggle {
        background: #262626;
        color: #E5E5E5;
        border: 1px solid #454545;
        border-radius: 4px;
        padding: 5px 8px;
    }
    QWidget#mediaDetailContent QPushButton:hover,
    QToolButton#mediaSeasonToggle:hover, QToolButton#mediaTechnicalToggle:hover { background: #383838; }
    QWidget#mediaDetailContent QPushButton:disabled,
    QWidget#mediaDetailContent QComboBox:disabled { color: #888888; }
    QWidget#mediaDetailContent QPushButton#mediaEpisodeWatchedButton:checked {
        background: #293831;
        color: #B8D9C1;
        border-color: #3F5949;
    }
    QToolButton#mediaDetailBack { background: transparent; border: none; padding: 6px; }
    QToolButton#mediaDetailBack:hover, QToolButton#mediaDetailBack:focus {
        background: rgba(25, 25, 25, 110);
        border-radius: 4px;
    }
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

class KeywordLayout final : public QLayout {
public:
    explicit KeywordLayout(QWidget *parent) : QLayout(parent) {
        setContentsMargins(0, 0, 0, 0);
        setSpacing(6);
    }

    ~KeywordLayout() override {
        qDeleteAll(items);
    }

    void addItem(QLayoutItem *item) override {
        items.append(item);
        invalidate();
    }

    int count() const override { return static_cast<int>(items.size()); }
    QLayoutItem *itemAt(int index) const override { return items.value(index); }
    QLayoutItem *takeAt(int index) override {
        if (index < 0 || index >= count()) {
            return nullptr;
        }
        auto *item = items.takeAt(index);
        invalidate();
        return item;
    }

    QSize sizeHint() const override {
        QSize hint(0, 0);
        for (const auto *item : items) {
            hint = hint.expandedTo(item->sizeHint());
        }
        return hint;
    }

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override { return arrange(QRect(0, 0, width, 0), false); }
    void setGeometry(const QRect &bounds) override {
        QLayout::setGeometry(bounds);
        arrange(bounds, true);
    }

private:
    int arrange(const QRect &bounds, const bool placeItems) const {
        int x = 0;
        int y = 0;
        int rowHeight = 0;
        for (auto *item : items) {
            const QSize hint = item->sizeHint();
            const int width = qBound(1, hint.width(), qMax(1, bounds.width()));
            const int height = qMax(hint.height(), item->heightForWidth(width));
            if (x > 0 && x + width > bounds.width()) {
                x = 0;
                y += rowHeight + spacing();
                rowHeight = 0;
            }
            if (placeItems) {
                item->setGeometry(QRect(bounds.topLeft() + QPoint(x, y), QSize(width, height)));
            }
            x += width + spacing();
            rowHeight = qMax(rowHeight, height);
        }
        return y + rowHeight;
    }

    QList<QLayoutItem *> items;
};
}

class MediaDetailHero final : public QWidget {
public:
    explicit MediaDetailHero(QWidget *parent) : QWidget(parent) {
        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        policy.setHeightForWidth(true);
        setSizePolicy(policy);
    }

    void setBackdrop(const QPixmap &image, const QPixmap &softImage = {}) {
        backdrop = image;
        softenedBackdrop = softImage;
        overlay = {};
        update();
    }

    void setTextPanel(QWidget *panel) { textPanel = panel; }

    int heightForWidth(const int width) const override {
        return qMax(minimumHeight(), QWidget::heightForWidth(width));
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        const QRect bounds = rect();
        if (backdrop.isNull()) {
            QLinearGradient background(0, 0, 0, height());
            background.setColorAt(0, QColor("#303030"));
            background.setColorAt(1, BACKGROUND);
            painter.fillRect(bounds, background);
            return;
        }
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        QRect source(QPoint(), size().scaled(backdrop.size(), Qt::KeepAspectRatio));
        source.moveCenter(backdrop.rect().center());
        painter.drawPixmap(bounds, backdrop, source);

        const QRect panel(textPanel->mapTo(this, QPoint()), textPanel->size());
        const QRect shadeBounds = panel.adjusted(-30, -24, 30, 24).intersected(bounds);
        if (!shadeBounds.isEmpty()) {
            if (overlay.isNull() || overlayBounds != shadeBounds || overlayHeroSize != size()) {
                updateOverlay(shadeBounds);
            }
            painter.drawImage(shadeBounds.topLeft(), overlay);
        }

        const int fadeStart = height() - FADE_HEIGHT;
        QLinearGradient fade(0, fadeStart, 0, height() - 1);
        fade.setColorAt(0, QColor(30, 30, 30, 0));
        fade.setColorAt(1, BACKGROUND);
        painter.fillRect(QRect(0, fadeStart, width(), FADE_HEIGHT), fade);
    }

private:
    void updateOverlay(const QRect &bounds) {
        overlayBounds = bounds;
        overlayHeroSize = size();
        overlay = QImage(bounds.size(), QImage::Format_ARGB32_Premultiplied);
        overlay.fill(Qt::transparent);
        QPainter painter(&overlay);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        if (!softenedBackdrop.isNull()) {
            QRect softSource(QPoint(), size().scaled(softenedBackdrop.size(), Qt::KeepAspectRatio));
            softSource.moveCenter(softenedBackdrop.rect().center());
            painter.drawPixmap(rect().translated(-bounds.topLeft()), softenedBackdrop, softSource);
        }
        painter.fillRect(overlay.rect(), QColor(0, 0, 0, 150));
        painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        QLinearGradient horizontal(0, 0, bounds.width(), 0);
        const qreal side = qMin(0.5, 30.0 / bounds.width());
        horizontal.setColorAt(0, Qt::transparent);
        horizontal.setColorAt(side, Qt::white);
        horizontal.setColorAt(1 - side, Qt::white);
        horizontal.setColorAt(1, Qt::transparent);
        painter.fillRect(overlay.rect(), horizontal);

        QLinearGradient vertical(0, 0, 0, bounds.height());
        const qreal edge = qMin(0.5, 24.0 / bounds.height());
        vertical.setColorAt(0, Qt::transparent);
        vertical.setColorAt(edge, Qt::white);
        vertical.setColorAt(1 - edge, Qt::white);
        vertical.setColorAt(1, Qt::transparent);
        painter.fillRect(overlay.rect(), vertical);
    }

    QWidget *textPanel = nullptr;
    QPixmap backdrop;
    QPixmap softenedBackdrop;
    QImage overlay;
    QRect overlayBounds;
    QSize overlayHeroSize;
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
    heroLayout->setContentsMargins(36, 18, 36, FADE_HEIGHT + 6);
    heroLayout->setSpacing(12);
    auto *back = new QToolButton(hero);
    back->setObjectName("mediaDetailBack");
    back->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    back->setAccessibleName("Zurück zur Liste");
    back->setToolTip("Zurück zur Liste");
    back->setCursor(Qt::PointingHandCursor);
    heroLayout->addWidget(back, 0, Qt::AlignLeft);
    heroLayout->addStretch(1);
    connect(back, &QToolButton::clicked, this, &MediaDetailView::backRequested);

    heading = new QHBoxLayout();
    heading->setSpacing(32);
    cover = new MediaImage(hero, Qt::KeepAspectRatio);
    cover->setAccessibleName("Poster vergrößern");
    cover->setCursor(Qt::PointingHandCursor);
    heading->addWidget(cover, 0, Qt::AlignLeft | Qt::AlignBottom);
    connect(cover, &MediaImage::clicked, this, [this] {
        showMediaImage(this, mediaImageFile(mediaImagePath(media, "poster", media.posterPath)));
    });

    auto *introPanel = new QWidget(hero);
    introPanel->setMaximumWidth(790);
    hero->setTextPanel(introPanel);
    auto *intro = new QVBoxLayout(introPanel);
    intro->setContentsMargins(0, 0, 0, 0);
    intro->setSpacing(7);
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
    noteButton = new QPushButton("Notiz hinzufügen", hero);
    noteButton->setCursor(Qt::PointingHandCursor);
    actions->addWidget(noteButton, 0, Qt::AlignBottom | Qt::AlignLeft);
    actions->addStretch(1);
    intro->addLayout(actions);
    heading->addWidget(introPanel, 1, Qt::AlignBottom);
    heroLayout->addLayout(heading);
    layout->addWidget(hero);

    auto *body = new QVBoxLayout();
    body->setContentsMargins(36, 0, 36, 28);
    body->setSpacing(18);
    noteSection = new QWidget(content);
    auto *noteLayout = new QVBoxLayout(noteSection);
    noteLayout->setContentsMargins(0, 0, 0, 0);
    noteLayout->setSpacing(6);
    noteLayout->addWidget(sectionTitle("Persönliche Notiz", noteSection));
    noteText = mediaText({}, noteSection);
    noteText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    noteLayout->addWidget(noteText);
    noteSection->hide();
    body->addWidget(noteSection);

    series = new MediaSeriesView(apiClient, content);
    body->addWidget(series);

    gallerySection = new QWidget(content);
    auto *galleryLayout = new QVBoxLayout(gallerySection);
    galleryLayout->setContentsMargins(0, 0, 0, 0);
    galleryLayout->addWidget(sectionTitle("Weitere Bilder", gallerySection));
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
    auto *keywordTags = new QWidget(keywordSection);
    keywords = new KeywordLayout(keywordTags);
    keywordLayout->addWidget(keywordTags);
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
    connect(noteButton, &QPushButton::clicked, this, [this, &apiClient] {
        openNoteDialog(apiClient);
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
                        media.personalNotes = pendingNotes;
                    }
                    noteButton->setEnabled(true);
                    updateNote();
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
    const QSize backdropSize(1920, 1440);
    loadMediaPixmap(*this, backdropFile, backdropSize, [this, imageRequest, backdropFile, backdropSize](const QPixmap &image) {
        if (imageRequest != backdropRequestId || image.isNull()) {
            return;
        }
        hero->setBackdrop(image);
        loadMediaPixmap(*this, backdropFile, backdropSize, [this, imageRequest, image](const QPixmap &softImage) {
            if (imageRequest == backdropRequestId) {
                hero->setBackdrop(image, softImage);
            }
        }, true);
    });

    const bool statusPending = statusRequestId != 0 && media.entityId == statusEntityId;
    watchStatus->setEnabled(statusRequestId == 0);
    watchStatus->setCurrentIndex(watchStatus->findData(statusPending ? pendingWatchStatus : media.watchStatusId));
    noteButton->setEnabled(personalRequestId == 0);
    updateNote();

    QStringList galleryFiles;
    for (int index = 0; index < qMin<qsizetype>(media.additionalBackdropPaths.size(), 4); ++index) {
        if (media.additionalBackdropPaths[index] == media.backdropPath) {
            continue;
        }
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
    const bool narrow = width < 620;
    hero->setMinimumHeight(narrow ? 460 : 550);
    heading->setDirection(narrow ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    heading->setSpacing(narrow ? 18 : 32);
    const int posterWidth = width >= 950 ? 200 : narrow ? 112 : 156;
    cover->setFixedSize(posterWidth, posterWidth * 3 / 2);
    actions->setDirection(width < 820 ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
}

void MediaDetailView::setRating(const std::optional<MediaRating> value) {
    savedRating = value;
    rating->setCurrentIndex(value ? rating->findData(static_cast<int>(*value)) : 0);
}

void MediaDetailView::updateNote() {
    const bool hasNote = !media.personalNotes.isEmpty();
    noteText->setText(media.personalNotes);
    noteSection->setVisible(hasNote);
    noteButton->setText(hasNote ? "Notiz bearbeiten" : "Notiz hinzufügen");
}

void MediaDetailView::openNoteDialog(ApiClient &apiClient) {
    QDialog dialog(this);
    dialog.setWindowTitle(noteButton->text());
    auto *layout = new QVBoxLayout(&dialog);
    auto *editor = new QPlainTextEdit(media.personalNotes, &dialog);
    editor->setAccessibleName("Persönliche Notiz");
    layout->addWidget(editor);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText("Speichern");
    buttons->button(QDialogButtonBox::Cancel)->setText("Abbrechen");
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (!media.personalNotes.isEmpty()) {
        auto *remove = buttons->addButton("Notiz entfernen", QDialogButtonBox::DestructiveRole);
        connect(remove, &QPushButton::clicked, &dialog, [editor, &dialog] {
            editor->clear();
            dialog.accept();
        });
    }
    prepareDialog(dialog);
    editor->setFocus();
    if (dialog.exec() != QDialog::Accepted || editor->toPlainText() == media.personalNotes) {
        return;
    }

    personalEntityId = media.entityId;
    pendingNotes = editor->toPlainText();
    const QJsonValue value = pendingNotes.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(pendingNotes);
    personalRequestId = apiClient.updateMediaPersonalData(media.entityId, {{"personal_notes", value}});
    noteButton->setEnabled(false);
}

void MediaDetailView::updateKeywords() {
    const bool showAll = moreKeywords->isChecked();
    const qsizetype count = showAll ? media.keywords.size()
        : qMin<qsizetype>(media.keywords.size(), KEYWORD_PREVIEW);
    while (auto *item = keywords->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    for (qsizetype index = 0; index < count; ++index) {
        auto *tag = new QLabel(media.keywords[index], keywords->parentWidget());
        tag->setObjectName("mediaKeyword");
        tag->setTextFormat(Qt::PlainText);
        tag->setWordWrap(true);
        keywords->addWidget(tag);
    }
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
