#ifndef FRONTEND_MEDIAIMAGES_H
#define FRONTEND_MEDIAIMAGES_H

#include <QAbstractButton>
#include <QPixmap>
#include <QStringList>

#include <functional>
#include <vector>

class QGridLayout;
struct Media;

QString mediaImagePath(const Media &media, const QString &name, const QString &tmdbPath);
QString mediaImageFile(const QString &relativePath);
void loadMediaPixmap(QObject &owner, const QString &file, const QSize &maximumSize,
                     std::function<void(const QPixmap &)> ready, bool soften = false);
void showMediaImages(QWidget *parent, const QStringList &files, int index = 0);

class MediaImage final : public QAbstractButton {
public:
    explicit MediaImage(QWidget *parent, Qt::AspectRatioMode mode = Qt::KeepAspectRatioByExpanding);
    void setFile(const QString &file, const QSize &maximumSize);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Qt::AspectRatioMode mode;
    QPixmap image;
    int requestId = 0;
};

class MediaGallery final : public QWidget {
public:
    explicit MediaGallery(QWidget *parent);
    void setFiles(const QStringList &files);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void arrangeImages();

    QStringList files;
    QGridLayout *layout;
    std::vector<MediaImage *> images;
    int columns = 0;
};

#endif // FRONTEND_MEDIAIMAGES_H
