#include "src/qpdfjswindow.h"
#include <QApplication>
#include <QSettings>
#include <QFileInfo>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationName("yshurik");
    a.setOrganizationDomain("yshurik.github.io");
    a.setApplicationName("QPdfJs");
    a.setApplicationVersion("1");

    QSettings s;
    QVector<QWidget *> windows;

    QString pdf_path = "/Users/mpogue/Dropbox/__squareDanceMusic/reference/Basic Definitions (17-08-13).pdf";
    QPdfJsWindow * window = new QPdfJsWindow(pdf_path);
    int w = s.value("geomw", 1000).toInt();
    int h = s.value("geomh", 800).toInt();
    window->resize(w,h);
    window->show();
    windows.push_back(window);

    int code = a.exec();

    for (auto window : windows) {
        if (window) {
            s.setValue("geomw", window->width());
            s.setValue("geomh", window->height());
        }
        delete window;
    }

    return code;
}
