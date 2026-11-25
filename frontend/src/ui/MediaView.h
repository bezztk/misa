#ifndef FRONTEND_MEDIAVIEW_H
#define FRONTEND_MEDIAVIEW_H

#include <QWidget>

#include <vector>

class ApiClient;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QStackedWidget;
struct Media;

class MediaView final : public QWidget {
public:
    explicit MediaView(ApiClient &apiClient, QWidget *parent = nullptr);
    void refresh();
    void openImportDialog();

private:
    void showMedia(const std::vector<Media> &media);
    void applyFilters();

    ApiClient &apiClient;
    QLineEdit *search;
    QComboBox *typeFilter;
    QStackedWidget *content;
    QListWidget *grid;
    QLabel *notice;
    int requestId = 0;
    bool mediaLoaded = false;
};

#endif // FRONTEND_MEDIAVIEW_H
