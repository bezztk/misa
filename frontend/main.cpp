#include <QApplication>
#include <QColor>
#include <QPalette>
#include "src/ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QPalette palette = app.palette();
    palette.setColor(QPalette::Mid, QColor("#969696"));
    app.setPalette(palette);

    MainWindow window;
    window.show();

    return app.exec();
}
