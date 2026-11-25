#ifndef FRONTEND_MEDIAVIEW_H
#define FRONTEND_MEDIAVIEW_H

#include "../models/Media.h"

#include <QWidget>

#include <vector>

class ApiClient;
class MediaDetailView;
class QComboBox;
class QEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QPoint;
class QStackedWidget;

class MediaView final : public QWidget {
public:
    explicit MediaView(ApiClient &apiClient, QWidget *parent = nullptr);
    void refresh();
    void openImportDialog();
    void showOverview();

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showMedia(const std::vector<Media> &media);
    void showDetails(int entityId);
    void applyFilters();
    void showCardContextMenu(const QPoint &position);

    ApiClient &apiClient;
    QStackedWidget *pages;
    QWidget *overview;
    MediaDetailView *detail;
    QLineEdit *search;
    QComboBox *typeFilter;
    QComboBox *watchStatusFilter;
    QStackedWidget *content;
    QListWidget *grid;
    QLabel *notice;
    std::vector<Media> media;
    int requestId = 0;
    bool mediaLoaded = false;
};

#endif // FRONTEND_MEDIAVIEW_H
