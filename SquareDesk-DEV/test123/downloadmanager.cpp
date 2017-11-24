#include "downloadmanager.h"

Downloader::Downloader(QObject *parent) :
    QObject(parent)
{
    networkReply = 0;
}

void Downloader::doDownload(QUrl url, QString d)
{
    destPath = d;
    QNetworkRequest req(url);
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this,SLOT(replyFinished(QNetworkReply*)));
    networkReply = manager->get(req);
}

void Downloader::abortTransfer() {
    // abort the transfer in progress...
//    qDebug() << "Aborting the transfer...";
    if (networkReply) {
        networkReply->abort();
    }
}

void Downloader::replyFinished(QNetworkReply *reply)
{
//    qDebug() << "Downloader::replyFinished()";
    if(reply->error()) {
//        qDebug() << "Downloader -- ERROR:" << reply->errorString();
    }
    else {
        QFile file(destPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {  // delete, if it already exists
            file.write(reply->readAll());
            file.flush();
            file.close();
//            qDebug() << "Downloader -- Temp File name: "<< file.fileName();
//            qDebug() << "Downloader -- Downloaded file size:" << file.size() << "Bytes";
        }
    }

    reply->deleteLater();
    manager->deleteLater();
    emit downloadFinished();
}
