#include <QTimer>
#include <QApplication>
#include <QWebEngineView>
#include <QWebChannel>
#include <QFile>
#include <QDir>

#include "src/qpdfjswindow.h"
#include "src/communicator.h"

QPdfJsWindow::QPdfJsWindow(QString pdf_path
                           ,QWidget *parent) :
    QMainWindow(parent)
{
    QString app_path = qApp->applicationDirPath();
    auto url = QUrl::fromLocalFile(app_path+"/minified/web/viewer.html");

    QDir dir(app_path+"/minified/web/");
    pdf_path = dir.relativeFilePath(pdf_path);

    m_communicator = new Communicator(this);
    m_communicator->setUrl(pdf_path);

    m_webView = new QWebEngineView(this);

    QWebChannel * channel = new QWebChannel(this);
    channel->registerObject(QStringLiteral("communicator"), m_communicator);
    m_webView->page()->setWebChannel(channel);

    m_webView->load(url);
    setCentralWidget(m_webView);
}

QPdfJsWindow::~QPdfJsWindow() {
}
