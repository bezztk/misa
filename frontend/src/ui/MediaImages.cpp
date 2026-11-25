#include "MediaImages.h"

#include "../models/Media.h"
#include "DialogUtils.h"

#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QPushButton>
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
            return image.scaled(image.size() / 4, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                .scaled(image.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        return image;
    }));
}

MediaImage::MediaImage(QWidget *parent, const Qt::AspectRatioMode mode)
    : QAbstractButton(parent), mode(mode) {
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
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

void showMediaImages(QWidget *parent, const QStringList &files, int index) {
    if (files.isEmpty()) {
        return;
    }

    QDialog dialog(parent);
    dialog.setWindowTitle("Bildansicht");
    auto *layout = new QVBoxLayout(&dialog);
    auto *picture = new MediaImage(&dialog, Qt::KeepAspectRatio);
    picture->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(picture, 1);

    auto *buttons = new QHBoxLayout();
    auto *previous = new QPushButton("Zurück", &dialog);
    auto *next = new QPushButton("Weiter", &dialog);
    auto *position = new QLabel(&dialog);
    auto *close = new QPushButton("Schließen", &dialog);
    buttons->addWidget(previous);
    buttons->addWidget(position);
    buttons->addWidget(next);
    buttons->addStretch(1);
    buttons->addWidget(close);
    layout->addLayout(buttons);
    previous->setVisible(files.size() > 1);
    next->setVisible(files.size() > 1);
    position->setVisible(files.size() > 1);

    const auto showImage = [&] {
        picture->setFile(files[index], QSize(1800, 1400));
        previous->setEnabled(index > 0);
        next->setEnabled(index + 1 < files.size());
        position->setText(QString("%1 / %2").arg(index + 1).arg(files.size()));
    };
    QObject::connect(previous, &QPushButton::clicked, &dialog, [&] { --index; showImage(); });
    QObject::connect(next, &QPushButton::clicked, &dialog, [&] { ++index; showImage(); });
    QObject::connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);
    prepareDialog(dialog);
    const QSize available = dialog.screen()->availableGeometry().size();
    dialog.resize(qMin(1040, available.width() * 9 / 10), qMin(740, available.height() * 9 / 10));
    showImage();
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
        auto *image = new MediaImage(this);
        image->setAccessibleName(QString("Galeriebild %1 öffnen").arg(index + 1));
        image->setCursor(Qt::PointingHandCursor);
        image->setFile(files[index], QSize(560, 360));
        connect(image, &MediaImage::clicked, this, [this, index] {
            showMediaImages(this, files, index);
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
    const int newColumns = qBound(1, (width() + spacing) / (180 + spacing), 4);
    const int imageWidth = qBound(1, (width() - spacing * (newColumns - 1)) / newColumns, 420);
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
