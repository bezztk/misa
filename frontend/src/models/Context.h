#ifndef FRONTEND_CONTEXT_H
#define FRONTEND_CONTEXT_H

#include <QByteArray>
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

struct Context {
    int id;
    QString title;
    QByteArray icon;
    QColor color;
    qint64 entityCount;
};

inline QIcon contextIcon(const QByteArray &png) {
    QPixmap pixmap;
    if (!png.isEmpty()) {
        pixmap.loadFromData(png, "PNG");
    }
    return QIcon(pixmap);
}

inline QColor contextTint(QColor color) {
    color.setAlpha(36);
    return color;
}

#endif // FRONTEND_CONTEXT_H
