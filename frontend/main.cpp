#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QPalette>
#include "src/ui/ContextMenuStyle.h"
#include "src/ui/MainWindow.h"

#ifdef Q_OS_WIN
#include <shobjidl.h>
#endif

int main(int argc, char* argv[]) {
#ifdef Q_OS_WIN
    SetCurrentProcessExplicitAppUserModelID(L"Misa.Desktop");
#endif

    QApplication app(argc, argv);
    QApplication::setApplicationName("misa");
    installContextMenuStyle();
    app.setWindowIcon(QIcon(":/assets/icons/misa.png"));

    QPalette palette = app.palette();
    palette.setColor(QPalette::Mid, QColor("#969696"));
    app.setPalette(palette);

    MainWindow window;
    window.setWindowIcon(app.windowIcon());
    window.show();

    return app.exec();
}
