#include "mainwindow.h"
#include <QApplication>
#include <QProcess>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

//    // start sd as a process -----
//    QString program = "/Users/mpogue/Documents/QtProjects/build-sd_qt-Desktop_Qt_5_7_0_clang_64bit-Debug/sd_qt";
////    QString program = "echo";
//    QStringList arguments;
//    arguments << "plus";

//    qDebug() << "starting process...";
//    QProcess *myProcess = new QProcess(Q_NULLPTR);
////    myProcess->setStandardOutputFile("/Users/mpogue/Documents/QtProjects/build-sdApp-Desktop_Qt_5_7_0_clang_64bit-Debug/foobar.txt");
//    myProcess->setStandardInputFile("/Users/mpogue/Documents/QtProjects/build-sdApp-Desktop_Qt_5_7_0_clang_64bit-Debug/in.txt");
//    myProcess->setWorkingDirectory("/Users/mpogue/Documents/QtProjects/build-sd_qt-Desktop_Qt_5_7_0_clang_64bit-Debug");
//    myProcess->start(program, arguments);

//    qDebug() << "waiting...";

//    myProcess->waitForStarted();
//    qDebug() << "started";

////    myProcess->write("heads start\n");
////    myProcess->waitForBytesWritten();
//    qDebug() << "first command given.  Waiting for response.";

//    while(myProcess->waitForReadyRead(1000)) {
//        qDebug() << "read:" << myProcess->readAll();
//    }

//    // TODO: hook up signals and slots
//    // TODO: in UI, when bytes come back from the process, append() them to the text area
//    // TODO: in UI, when new bytes are ready from the input line, OR better yet when a Console key is pressed
//    //   here's a Console: http://doc.qt.io/qt-5/qtserialport-terminal-console-cpp.html
//    // TODO: sdtty clears the screen by just repeating the previous 25 lines, which are buffered.  Stop that.

    return a.exec();
}
