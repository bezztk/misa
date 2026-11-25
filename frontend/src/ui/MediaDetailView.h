#ifndef FRONTEND_MEDIADETAILVIEW_H
#define FRONTEND_MEDIADETAILVIEW_H

#include "../models/Media.h"

#include <QScrollArea>

class ApiClient;
class MediaDescription;
class MediaGallery;
class MediaImage;
class MediaSeriesView;
class QBoxLayout;
class QComboBox;
class QFormLayout;
class QLabel;
class MediaDetailHero;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QToolButton;
class QWidget;

class MediaDetailView final : public QScrollArea {
    Q_OBJECT

public:
    explicit MediaDetailView(ApiClient &apiClient, QWidget *parent = nullptr);
    void setMedia(const Media &media);
    void updateCompletionDate(int entityId, const QDateTime &completedAt);

signals:
    void backRequested();

private:
    void resizeEvent(QResizeEvent *event) override;
    void setRating(std::optional<MediaRating> value);
    void updateHeaderLayout();
    void updateKeywords();
    void setTechnicalDetails();
    void updatePersonalSaveButton();

    Media media{};
    MediaDetailHero *hero;
    QBoxLayout *heading = nullptr;
    QBoxLayout *actions;
    MediaImage *cover;
    QLabel *mediaTypeLabel;
    QLabel *title;
    QLabel *originalTitle;
    QLabel *facts;
    QLabel *genres;
    MediaDescription *description;
    QLabel *tmdbRating;
    QComboBox *watchStatus;
    QComboBox *rating;
    QPushButton *trailer;
    MediaSeriesView *series;
    QSpinBox *watchCount;
    QLabel *completionDate;
    QPlainTextEdit *notes;
    QPushButton *savePersonal;
    QWidget *gallerySection;
    MediaGallery *gallery;
    QLabel *releaseDate;
    QWidget *keywordSection;
    QLabel *keywords;
    QToolButton *moreKeywords;
    QToolButton *technicalToggle;
    QFormLayout *technicalDetails;
    int backdropRequestId = 0;
    int statusRequestId = 0;
    int statusEntityId = 0;
    int pendingWatchStatus = 0;
    int personalRequestId = 0;
    int personalEntityId = 0;
    int pendingWatchCount = 0;
    QString pendingNotes;
    int ratingUpdateEntityId = 0;
    std::optional<MediaRating> savedRating;
};

#endif // FRONTEND_MEDIADETAILVIEW_H
