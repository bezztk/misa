#include "MediaImages.h"

#include "../models/Media.h"
#include "DialogUtils.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QGridLayout>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QScreen>
#include <QStandardPaths>
#include <QStyle>
#include <QVBoxLayout>
#include <QtConcurrentRun>

#include <utility>

QString mediaImagePath(const Media &media, const QString &name, const QString &tmdbPath) {
    const QString extension = QFileInfo(tmdbPath).suffix();
    if (extension.isEmpty() || (media.mediaType != "movie" && media.mediaType != "tv")) {
        return {};
    }
    return QString("%1/%2/%3.%4").arg(media.mediaType, QString::number(media.tmdbId), name, extension);
}

QString mediaImageFile(const QString &relativePath) {
    if (relativePath.isEmpty()) {
        return {};
    }
    const QDir appData(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    return appData.filePath("media/" + relativePath);
}

void loadMediaPixmap(
    QObject &owner, const QString &file, const QSize &maximumSize,
    std::function<void(const QPixmap &)> ready, const bool soften
) {
    if (file.isEmpty()) {
        ready({});
        return;
    }
    const QString cacheKey = QString("media|%1|%2|%3x%4|%5")
        .arg(file).arg(QFileInfo(file).lastModified().toMSecsSinceEpoch())
        .arg(maximumSize.width()).arg(maximumSize.height()).arg(soften);
    QPixmap cached;
    if (QPixmapCache::find(cacheKey, &cached)) {
        ready(cached);
        return;
    }

    auto *watcher = new QFutureWatcher<QImage>(&owner);
    QObject::connect(watcher, &QFutureWatcher<QImage>::finished, &owner,
        [watcher, cacheKey, ready = std::move(ready)] {
            const QPixmap pixmap = QPixmap::fromImage(watcher->result());
            watcher->deleteLater();
            if (!pixmap.isNull()) {
                QPixmapCache::insert(cacheKey, pixmap);
            }
            ready(pixmap);
        });
    watcher->setFuture(QtConcurrent::run([file, maximumSize, soften] {
        QImageReader reader(file);
        const QSize size = reader.size();
        if (size.width() > maximumSize.width() || size.height() > maximumSize.height()) {
            reader.setScaledSize(size.scaled(maximumSize, Qt::KeepAspectRatio));
        }
        const QImage image = reader.read();
        if (soften && image.width() >= 4 && image.height() >= 4) {
            return image.scaled(image.size() / 4, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        return image;
    }));
}

MediaImage::MediaImage(QWidget *parent, const Qt::AspectRatioMode mode)
    : QAbstractButton(parent), mode(mode) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void MediaImage::setFile(const QString &file, const QSize &maximumSize) {
    const int currentRequest = ++requestId;
    image = {};
    update();
    loadMediaPixmap(*this, file, maximumSize, [this, currentRequest](const QPixmap &pixmap) {
        if (currentRequest == requestId) {
            image = pixmap;
            update();
        }
    });
}

void MediaImage::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath outline;
    outline.addRoundedRect(rect(), 4, 4);
    painter.setClipPath(outline);
    painter.fillRect(rect(), QColor("#303030"));
    if (image.isNull()) {
        const QRect icon(rect().center() - QPoint(14, 14), QSize(28, 28));
        style()->standardIcon(QStyle::SP_MediaPlay).paint(&painter, icon, Qt::AlignCenter, QIcon::Disabled);
        return;
    }

    if (mode == Qt::KeepAspectRatioByExpanding) {
        QRect source(QPoint(), size().scaled(image.size(), Qt::KeepAspectRatio));
        source.moveCenter(image.rect().center());
        painter.drawPixmap(rect(), image, source);
    } else {
        QRect target(QPoint(), image.size().scaled(size(), Qt::KeepAspectRatio));
        target.moveCenter(rect().center());
        painter.drawPixmap(target, image);
    }
}

void showMediaImage(QWidget *parent, const QString &file) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Bildansicht");
    auto *layout = new QVBoxLayout(&dialog);
    auto *picture = new MediaImage(&dialog, Qt::KeepAspectRatio);
    picture->setFocusPolicy(Qt::NoFocus);
    picture->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(picture, 1);
    auto *buttons = new QDialogButtonBox(&dialog);
    buttons->addButton("Schließen", QDialogButtonBox::AcceptRole);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    prepareFormDialog(dialog, *buttons);
    const QSize available = dialog.screen()->availableGeometry().size();
    dialog.resize(qMin(1040, available.width() * 9 / 10), qMin(740, available.height() * 9 / 10));
    picture->setFile(file, QSize(1800, 1400));
    dialog.exec();
}

MediaGallery::MediaGallery(QWidget *parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
}

void MediaGallery::setFiles(const QStringList &newFiles) {
    for (MediaImage *image : images) {
        delete image;
    }
    images.clear();
    files = newFiles;
    columns = 0;
    for (int index = 0; index < files.size(); ++index) {
        auto *image = new MediaImage(this, Qt::KeepAspectRatio);
        image->setAccessibleName(QString("Galeriebild %1 öffnen").arg(index + 1));
        image->setCursor(Qt::PointingHandCursor);
        image->setFile(files[index], QSize(1100, 720));
        connect(image, &MediaImage::clicked, this, [this, index] {
            showMediaImage(this, files[index]);
        });
        images.push_back(image);
    }
    arrangeImages();
}

void MediaGallery::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    arrangeImages();
}

void MediaGallery::arrangeImages() {
    if (images.empty()) {
        return;
    }
    const int spacing = layout->spacing();
    const int newColumns = width() >= 610 && images.size() > 1 ? 2 : 1;
    const int imageWidth = qMax(1, (width() - spacing * (newColumns - 1)) / newColumns);
    if (columns != newColumns) {
        columns = newColumns;
        for (int index = 0; index < static_cast<int>(images.size()); ++index) {
            layout->removeWidget(images[index]);
            layout->addWidget(images[index], index / columns, index % columns);
        }
    }
    for (MediaImage *image : images) {
        image->setFixedSize(imageWidth, qMax(1, imageWidth * 9 / 16));
    }
}
