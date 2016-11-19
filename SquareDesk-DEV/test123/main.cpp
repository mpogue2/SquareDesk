#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("SquareDesk");
    a.setOrganizationName("Zenstar Software");
    a.setOrganizationDomain("zenstarstudio.com");

    MainWindow w;

    // put window back where it was last time (modulo the screen size, which
    //   is automatically taken care of.
    QSettings settings;
    w.restoreGeometry(settings.value("geometry").toByteArray());
    w.restoreState(settings.value("windowState").toByteArray());

    w.show();

    // http://stackoverflow.com/questions/6516299/qt-c-icons-not-showing-up-when-program-is-run-under-windows-o-s
    QString sDir = QCoreApplication::applicationDirPath();
    a.addLibraryPath(sDir + "/plugins");

    a.installEventFilter(new GlobalEventFilter(w.ui));

    return a.exec();
}
