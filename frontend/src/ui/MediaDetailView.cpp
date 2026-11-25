#include "MediaDetailView.h"

#include "../api/ApiClient.h"

#include <QComboBox>
#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include <utility>

namespace {
const QColor BACKGROUND(30, 30, 30);
constexpr QSize COVER_SIZE(124, 186);
constexpr int FADE_HEIGHT = 80;

constexpr auto DETAIL_STYLE = R"(
    QWidget#mediaDetailContent { background: #1E1E1E; }
    QLabel#mediaDetailTitle {
        color: #F5F5F5;
        font-size: 32px;
        font-weight: 600;
    }
    QLabel#mediaDetailCover { background: #303030; }
    QLabel#mediaDetailMetadata { color: #B8B8B8; font-size: 14px; }
    QLabel#mediaDetailRatingLabel { color: #E5E5E5; font-size: 14px; }
    QComboBox#mediaDetailRating {
        background: #262626;
        color: #E5E5E5;
        border: 1px solid #3A3A3A;
        border-radius: 4px;
        padding: 5px 9px;
    }
    QLabel#mediaDetailDescription { color: #E5E5E5; font-size: 15px; }
    QToolButton#mediaDetailBack {
        background: #262626;
        color: #E5E5E5;
        border: 1px solid #454545;
        border-radius: 4px;
        padding: 6px 10px;
    }
    QToolButton#mediaDetailBack:hover { background: #383838; }
)";

QPixmap readImage(const QString &file, const QSize &maximumSize) {
    if (file.isEmpty()) {
        return {};
    }
    QImageReader reader(file);
    const QSize size = reader.size();
    if (size.width() > maximumSize.width() || size.height() > maximumSize.height()) {
        reader.setScaledSize(size.scaled(maximumSize, Qt::KeepAspectRatio));
    }
    return QPixmap::fromImage(reader.read());
}

QPixmap blurredBackdrop(const QString &file) {
    const QPixmap image = readImage(file, QSize(1600, 1600));
    if (image.isNull()) {
        return {};
    }

    QGraphicsScene scene;
    auto *picture = scene.addPixmap(image);
    auto *blur = new QGraphicsBlurEffect();
    blur->setBlurRadius(image.width() * 0.004);
    blur->setBlurHints(QGraphicsBlurEffect::QualityHint);
    picture->setGraphicsEffect(blur);

    QPixmap backdrop(image.size());
    backdrop.fill(BACKGROUND);
    QPainter painter(&backdrop);
    const QRectF bounds(image.rect());
    scene.render(&painter, bounds, bounds);
    painter.end();
    return backdrop;
}
}

class MediaDetailHero final : public QWidget {
public:
    explicit MediaDetailHero(QWidget *parent) : QWidget(parent) {
        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        policy.setHeightForWidth(true);
        setSizePolicy(policy);
    }

    void setBackdrop(QPixmap image) {
        backdrop = std::move(image);
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
        if (backdrop.isNull()) {
            return;
        }

        const int height = backdropHeight(width());
        const QRect imageBounds(0, 0, width(), height);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawPixmap(imageBounds, backdrop);

        QLinearGradient shade(0, 0, 0, height);
        shade.setColorAt(0.0, QColor(0, 0, 0, 38));
        shade.setColorAt(0.55, QColor(0, 0, 0, 38));
        shade.setColorAt(1.0, QColor(0, 0, 0, 155));
        painter.fillRect(imageBounds, shade);

        const int fadeStart = qMax(0, height - FADE_HEIGHT);
        QLinearGradient fade(0, fadeStart, 0, height);
        fade.setColorAt(0.0, QColor(30, 30, 30, 0));
        fade.setColorAt(0.55, QColor(30, 30, 30, 155));
        fade.setColorAt(1.0, BACKGROUND);
        painter.fillRect(QRect(0, fadeStart, width(), height - fadeStart), fade);
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

    auto *heading = new QHBoxLayout();
    heading->setSpacing(16);
    cover = new QLabel(hero);
    cover->setObjectName("mediaDetailCover");
    cover->setFixedSize(COVER_SIZE);
    cover->setAlignment(Qt::AlignCenter);
    heading->addWidget(cover);

    title = new QLabel(hero);
    title->setObjectName("mediaDetailTitle");
    title->setTextFormat(Qt::PlainText);
    title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    title->setWordWrap(true);
    heading->addWidget(title, 1, Qt::AlignBottom);
    heroLayout->addLayout(heading);
    layout->addWidget(hero);

    auto *body = new QVBoxLayout();
    body->setContentsMargins(20, 0, 20, 30);
    body->setSpacing(12);
    metadata = new QLabel(content);
    metadata->setObjectName("mediaDetailMetadata");
    metadata->setTextFormat(Qt::PlainText);
    metadata->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    metadata->setWordWrap(true);
    body->addWidget(metadata);

    description = new QLabel(content);
    description->setObjectName("mediaDetailDescription");
    description->setTextFormat(Qt::PlainText);
    description->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    description->setWordWrap(true);
    body->addWidget(description);

    auto *ratingLayout = new QVBoxLayout();
    ratingLayout->setSpacing(5);
    auto *ratingLabel = new QLabel("&Persönliche Bewertung", content);
    ratingLabel->setObjectName("mediaDetailRatingLabel");
    ratingLayout->addWidget(ratingLabel);
    rating = new QComboBox(content);
    rating->setObjectName("mediaDetailRating");
    ratingLabel->setBuddy(rating);
    ratingLayout->addWidget(rating, 0, Qt::AlignLeft);
    body->addLayout(ratingLayout);

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
        if (value == savedRating) {
            return;
        }
        ratingUpdateEntityId = entityId;
        rating->setEnabled(false);
        apiClient.updateMediaRating(entityId, value);
    });
    connect(&apiClient, &ApiClient::mediaRatingUpdateFinished, this,
            [this](const int id, const std::optional<MediaRating> value, const bool successful) {
                if (id != ratingUpdateEntityId) {
                    return;
                }
                ratingUpdateEntityId = 0;
                if (id == entityId) {
                    setRating(successful ? value : savedRating);
                }
                rating->setEnabled(true);
            });

    layout->addLayout(body);
    layout->addStretch(1);
    setWidget(content);
}

void MediaDetailView::setMedia(
    const Media &media, const QString &posterFile, const QString &backdropFile
) {
    entityId = media.entityId;
    setRating(media.personalRating);
    title->setText(media.displayTitle());
    const QPixmap poster = readImage(posterFile, QSize(384, 576));
    if (poster.isNull()) {
        cover->setPixmap(style()->standardIcon(QStyle::SP_MediaPlay).pixmap(32, 32, QIcon::Disabled));
    } else {
        const qreal ratio = devicePixelRatioF();
        const QSize target(qRound(COVER_SIZE.width() * ratio), qRound(COVER_SIZE.height() * ratio));
        const QPixmap scaled = poster.scaled(target, Qt::KeepAspectRatioByExpanding,
                                             Qt::SmoothTransformation);
        QRect source(QPoint(), target);
        source.moveCenter(scaled.rect().center());
        QPixmap cropped = scaled.copy(source);
        cropped.setDevicePixelRatio(ratio);
        cover->setPixmap(cropped);
    }

    QStringList facts;
    if (media.releaseDate.isValid()) {
        facts.append(QString::number(media.releaseDate.year()));
    }
    if (!media.genres.isEmpty()) {
        facts.append(media.genres.join(", "));
    }
    if (media.mediaType == "movie" && media.runtimeMinutes) {
        facts.append(QString("%1 Min.").arg(*media.runtimeMinutes));
    }
    metadata->setText(facts.join("  ·  "));
    metadata->setVisible(!facts.isEmpty());
    description->setText(media.description);
    description->setVisible(!media.description.isEmpty());

    hero->setBackdrop(blurredBackdrop(backdropFile));
    verticalScrollBar()->setValue(0);
}

void MediaDetailView::setRating(const std::optional<MediaRating> value) {
    savedRating = value;
    rating->setCurrentIndex(value ? rating->findData(static_cast<int>(*value)) : 0);
}
