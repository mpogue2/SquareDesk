#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("SquareDesk");
    a.setOrganizationName("Zenstar Software");
    a.setOrganizationDomain("zenstarstudio.com");

    MainWindow w;
    w.show();

    // http://stackoverflow.com/questions/6516299/qt-c-icons-not-showing-up-when-program-is-run-under-windows-o-s
    QString sDir = QCoreApplication::applicationDirPath();
    a.addLibraryPath(sDir + "/plugins");

    a.installEventFilter(new GlobalEventFilter(w.ui));

    return a.exec();
}
