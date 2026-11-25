#ifndef FRONTEND_MEDIAUI_H
#define FRONTEND_MEDIAUI_H

#include <QColor>
#include <QComboBox>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

struct MediaWatchStatus {
    QString text;
    QColor color;
};

inline MediaWatchStatus mediaWatchStatus(const int id) {
    switch (id) {
    case 1: return {"Geplant", QColor("#B8B8B8")};
    case 2: return {"Wird angesehen", QColor("#9DBFF2")};
    case 3: return {"Abgeschlossen", QColor("#9DCCAE")};
    case 4: return {"Pausiert", QColor("#E1BC84")};
    case 5: return {"Abgebrochen", QColor("#E0A29D")};
    default: return {};
    }
}

inline void addMediaWatchStatuses(QComboBox &control) {
    for (int id = 1; id <= 5; ++id) {
        control.addItem(mediaWatchStatus(id).text, id);
    }
}

inline QLabel *mediaText(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    return label;
}

class MediaDescription final : public QWidget {
public:
    explicit MediaDescription(const int previewLength, QWidget *parent)
        : QWidget(parent), previewLength(previewLength) {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);
        label = mediaText({}, this);
        layout->addWidget(label);
        more = new QToolButton(this);
        more->setAutoRaise(true);
        more->setCheckable(true);
        more->setCursor(Qt::PointingHandCursor);
        layout->addWidget(more, 0, Qt::AlignLeft);
        connect(more, &QToolButton::toggled, this, [this] { refresh(); });
    }

    void setText(const QString &text) {
        description = text;
        more->setChecked(false);
        refresh();
    }

private:
    void refresh() {
        const bool longDescription = description.size() > previewLength;
        const bool expanded = more->isChecked();
        label->setText(description.isEmpty() ? QStringLiteral("Keine Beschreibung vorhanden.")
            : longDescription && !expanded ? description.left(previewLength).trimmed() + "…"
            : description);
        more->setVisible(longDescription);
        more->setText(expanded ? "Weniger anzeigen" : "Mehr anzeigen");
    }

    int previewLength;
    QLabel *label;
    QToolButton *more;
    QString description;
};

#endif // FRONTEND_MEDIAUI_H
