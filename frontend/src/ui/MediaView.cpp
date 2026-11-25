#include "MediaView.h"

#include "../api/ApiClient.h"
#include "MediaImportDialog.h"

#include <QActionGroup>
#include <QCache>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

namespace {
constexpr int CARD_WIDTH = 180;
constexpr int POSTER_HEIGHT = CARD_WIDTH * 3 / 2;
constexpr int CARD_HEIGHT = POSTER_HEIGHT + 78;
constexpr int CARD_SPACING = 16;

enum MediaRole {
    EntityIdRole = Qt::UserRole,
    OriginalTitleRole,
    MediaTypeRole,
    YearRole,
    PosterFileRole,
    WatchStatusRole,
    RatingRole
};

constexpr auto MEDIA_STYLE = R"(
    QWidget#mediaView, QListWidget#mediaGrid {
        background: #1E1E1E;
        border: none;
    }
    QLineEdit#mediaSearch, QComboBox#mediaFilter {
        background: #262626;
        color: #E5E5E5;
        border: 1px solid #3A3A3A;
        border-radius: 4px;
        padding: 5px 9px;
    }
    QLineEdit#mediaSearch:focus, QComboBox#mediaFilter:focus {
        border-color: #7FA7E2;
    }
    QLabel#mediaNotice {
        color: #A8A8A8;
    }
)";

struct WatchStatus {
    QString text;
    QColor color;
};

WatchStatus watchStatus(const int id) {
    switch (id) {
    case 1: return {"Geplant", QColor("#B8B8B8")};
    case 2: return {"Wird angesehen", QColor("#9DBFF2")};
    case 3: return {"Abgeschlossen", QColor("#9DCCAE")};
    case 4: return {"Pausiert", QColor("#E1BC84")};
    case 5: return {"Abgebrochen", QColor("#E0A29D")};
    default: return {};
    }
}

QString posterFile(const QDir &mediaDirectory, const Media &media) {
    const QString extension = QFileInfo(media.posterPath).suffix();
    if (extension.isEmpty() || (media.mediaType != "movie" && media.mediaType != "tv")) {
        return {};
    }
    return mediaDirectory.filePath(QString("%1/%2/poster.%3")
        .arg(media.mediaType, QString::number(media.tmdbId), extension));
}

class MediaCardDelegate final : public QStyledItemDelegate {
public:
    explicit MediaCardDelegate(QListWidget *grid)
        : QStyledItemDelegate(grid),
          placeholder(grid->style()->standardIcon(QStyle::SP_MediaPlay)) {}

    void clearPosters() { posters.clear(); }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override {
        return {CARD_WIDTH, CARD_HEIGHT};
    }

    void paint(
        QPainter *painter,
        const QStyleOptionViewItem &option,
        const QModelIndex &index
    ) const override {
        const QRect card(option.rect.topLeft(), QSize(CARD_WIDTH, CARD_HEIGHT));
        const QRect cover(card.topLeft(), QSize(CARD_WIDTH, POSTER_HEIGHT));
        QPainterPath outline;
        outline.addRoundedRect(card, 5, 5);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        painter->setClipPath(outline, Qt::IntersectClip);
        painter->fillRect(card, QColor("#262626"));
        painter->fillRect(cover, QColor("#303030"));

        const QPixmap poster = loadPoster(
            index.data(PosterFileRole).toString(), painter->device()->devicePixelRatioF()
        );
        if (poster.isNull()) {
            const QRect icon(cover.center() - QPoint(16, 16), QSize(32, 32));
            placeholder.paint(painter, icon, Qt::AlignCenter, QIcon::Disabled);
        } else {
            QRect source(QPoint(), cover.size().scaled(poster.size(), Qt::KeepAspectRatio));
            source.moveCenter(poster.rect().center());
            painter->drawPixmap(cover, poster, source);
        }

        const int left = card.left() + 10;
        const int width = card.width() - 20;
        const int top = cover.bottom() + 1;
        constexpr int textFlags = Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine;

        QFont titleFont = option.font;
        titleFont.setPixelSize(15);
        titleFont.setWeight(QFont::DemiBold);
        painter->setFont(titleFont);
        painter->setPen(QColor("#E5E5E5"));
        const QString title = QFontMetrics(titleFont).elidedText(
            index.data(Qt::DisplayRole).toString().simplified(), Qt::ElideRight, width
        );
        painter->drawText(QRect(left, top + 7, width, 21), textFlags, title);

        QFont metaFont = option.font;
        metaFont.setPixelSize(13);
        painter->setFont(metaFont);
        painter->setPen(QColor("#A8A8A8"));
        painter->drawText(
            QRect(left, top + 29, width, 17), textFlags, index.data(YearRole).toString()
        );

        const WatchStatus status = watchStatus(index.data(WatchStatusRole).toInt());
        const QRect statusRow(left, top + 52, width, 17);
        if (!status.text.isEmpty()) {
            painter->setBrush(status.color);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(QPointF(left + 3, statusRow.center().y()), 3, 3);
            painter->setPen(status.color);
            painter->drawText(statusRow.adjusted(11, 0, 0, 0), textFlags, status.text);
        }

        const QVariant rating = index.data(RatingRole);
        if (rating.isValid()) {
            painter->setPen(QColor("#E1BC84"));
            painter->drawText(
                statusRow, Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine,
                QString("★ %1").arg(rating.toInt())
            );
        }
        painter->restore();
    }

private:
    QPixmap loadPoster(const QString &file, const qreal devicePixelRatio) const {
        if (file.isEmpty()) {
            return {};
        }
        const QString key = file + '|' + QString::number(devicePixelRatio);
        if (const QPixmap *cached = posters.object(key)) {
            return *cached;
        }

        QImageReader reader(file);
        const QSize target(
            qRound(CARD_WIDTH * devicePixelRatio), qRound(POSTER_HEIGHT * devicePixelRatio)
        );
        if (const QSize source = reader.size(); source.isValid()) {
            reader.setScaledSize(source.scaled(target, Qt::KeepAspectRatioByExpanding));
        }
        const QPixmap poster = QPixmap::fromImage(reader.read());
        const int sizeKiB = qMax(1, poster.width() * poster.height() * 4 / 1024);
        posters.insert(key, new QPixmap(poster), sizeKiB);
        return poster;
    }

    QIcon placeholder;
    mutable QCache<QString, QPixmap> posters{32 * 1024};
};
}

MediaView::MediaView(ApiClient &apiClient, QWidget *parent)
    : QWidget(parent), apiClient(apiClient) {
    setObjectName("mediaView");
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet(MEDIA_STYLE);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 4, 0);
    layout->setSpacing(14);
    const QMargins margins = layout->contentsMargins();
    setMinimumWidth(CARD_WIDTH + CARD_SPACING + margins.left() + margins.right()
                    + style()->pixelMetric(QStyle::PM_ScrollBarExtent));

    auto *toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(0, 0, 14, 0);
    toolbar->setSpacing(10);
    search = new QLineEdit(this);
    search->setObjectName("mediaSearch");
    search->setPlaceholderText("Medienbestand durchsuchen");
    search->setAccessibleName("Medienbestand durchsuchen");
    search->setClearButtonEnabled(true);
    toolbar->addWidget(search, 1);

    typeFilter = new QComboBox(this);
    typeFilter->setObjectName("mediaFilter");
    typeFilter->setAccessibleName("Medientyp");
    typeFilter->addItem("Alle", QString());
    typeFilter->addItem("Filme", "movie");
    typeFilter->addItem("Serien", "tv");
    typeFilter->setMinimumWidth(100);
    toolbar->addWidget(typeFilter);
    layout->addLayout(toolbar);

    content = new QStackedWidget(this);
    notice = new QLabel("Medien werden geladen …", content);
    notice->setObjectName("mediaNotice");
    notice->setAlignment(Qt::AlignCenter);
    notice->setWordWrap(true);
    content->addWidget(notice);

    grid = new QListWidget(content);
    grid->setObjectName("mediaGrid");
    grid->setAccessibleName("Film- und Serienübersicht");
    grid->setViewMode(QListView::IconMode);
    grid->setFlow(QListView::LeftToRight);
    grid->setWrapping(true);
    grid->setResizeMode(QListView::Adjust);
    grid->setMovement(QListView::Static);
    grid->setGridSize(QSize(CARD_WIDTH + CARD_SPACING, CARD_HEIGHT + CARD_SPACING));
    grid->setUniformItemSizes(true);
    grid->setSelectionMode(QAbstractItemView::NoSelection);
    grid->setContextMenuPolicy(Qt::CustomContextMenu);
    grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    grid->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    grid->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    grid->setItemDelegate(new MediaCardDelegate(grid));
    content->addWidget(grid);
    layout->addWidget(content, 1);

    connect(search, &QLineEdit::textChanged, this, &MediaView::applyFilters);
    connect(typeFilter, &QComboBox::currentIndexChanged, this, &MediaView::applyFilters);
    connect(grid, &QListWidget::customContextMenuRequested,
            this, &MediaView::showCardContextMenu);
    connect(&apiClient, &ApiClient::mediaReceived, this,
            [this](const int id, const std::vector<Media> &media) {
                if (id == requestId) {
                    showMedia(media);
                }
            });
    connect(&apiClient, &ApiClient::mediaLoadFailed, this, [this](const int id) {
        if (id == requestId) {
            notice->setText("Der Medienbestand konnte nicht geladen werden.");
            content->setCurrentWidget(notice);
        }
    });
    connect(&apiClient, &ApiClient::mediaImported, this, &MediaView::refresh);
    connect(&apiClient, &ApiClient::mediaWatchStatusChanged, this, &MediaView::refresh);
}

void MediaView::refresh() {
    if (!mediaLoaded) {
        notice->setText("Medien werden geladen …");
    }
    requestId = apiClient.getMedia();
}

void MediaView::openImportDialog() {
    MediaImportDialog dialog(apiClient, window());
    dialog.exec();
}

void MediaView::showMedia(const std::vector<Media> &media) {
    const int scrollPosition = grid->verticalScrollBar()->value();
    mediaLoaded = true;
    const QDir appData(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    const QDir mediaDirectory(appData.filePath("media"));
    static_cast<MediaCardDelegate *>(grid->itemDelegate())->clearPosters();
    grid->clear();
    for (const Media &entry : media) {
        const QString title = entry.displayTitle();
        auto *item = new QListWidgetItem(title);
        item->setToolTip("<p>" + title.toHtmlEscaped() + "</p>");
        item->setData(EntityIdRole, entry.entityId);
        item->setData(OriginalTitleRole, entry.originalTitle);
        item->setData(MediaTypeRole, entry.mediaType);
        if (entry.releaseDate.isValid()) {
            item->setData(YearRole, QString::number(entry.releaseDate.year()));
        }
        item->setData(PosterFileRole, posterFile(mediaDirectory, entry));
        item->setData(WatchStatusRole, entry.watchStatusId);
        if (entry.personalRating) {
            item->setData(RatingRole, *entry.personalRating);
        }
        grid->addItem(item);
    }
    applyFilters();
    grid->doItemsLayout();
    grid->verticalScrollBar()->setValue(scrollPosition);
}

void MediaView::showCardContextMenu(const QPoint &position) {
    const QListWidgetItem *item = grid->itemAt(position);
    if (!item) {
        return;
    }
    const int entityId = item->data(EntityIdRole).toInt();
    const int currentStatus = item->data(WatchStatusRole).toInt();

    QMenu menu(this);
    QMenu *statusMenu = menu.addMenu("Sichtungsstatus");
    auto *statuses = new QActionGroup(statusMenu);
    for (int statusId = 1; statusId <= 5; ++statusId) {
        QAction *action = statusMenu->addAction(watchStatus(statusId).text);
        statuses->addAction(action);
        action->setData(statusId);
        action->setCheckable(true);
        action->setChecked(statusId == currentStatus);
    }

    if (const QAction *selected = menu.exec(grid->viewport()->mapToGlobal(position))) {
        const int statusId = selected->data().toInt();
        if (statusId != currentStatus) {
            apiClient.updateMediaWatchStatus(entityId, statusId);
        }
    }
}

void MediaView::applyFilters() {
    if (!mediaLoaded) {
        return;
    }
    const QString term = search->text().trimmed();
    const QString mediaType = typeFilter->currentData().toString();
    bool hasMatches = false;
    for (int row = 0; row < grid->count(); ++row) {
        QListWidgetItem *item = grid->item(row);
        const bool matchesType = mediaType.isEmpty()
            || item->data(MediaTypeRole).toString() == mediaType;
        const bool matchesTerm = term.isEmpty()
            || item->text().contains(term, Qt::CaseInsensitive)
            || item->data(OriginalTitleRole).toString().contains(term, Qt::CaseInsensitive);
        const bool visible = matchesType && matchesTerm;
        item->setHidden(!visible);
        hasMatches = hasMatches || visible;
    }
    if (hasMatches) {
        content->setCurrentWidget(grid);
    } else {
        notice->setText(grid->count() == 0
            ? "Noch keine Filme oder Serien importiert."
            : "Keine Medien gefunden.");
        content->setCurrentWidget(notice);
    }
}
