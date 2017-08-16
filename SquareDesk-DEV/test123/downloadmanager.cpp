#include "downloadmanager.h"

Downloader::Downloader(QObject *parent) :
    QObject(parent)
{
}

void Downloader::doDownload(QUrl url, QString d)
{
    destPath = d;
    QNetworkRequest req(url);
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this,SLOT(replyFinished(QNetworkReply*)));
    manager -> get(req);
}

void Downloader::replyFinished(QNetworkReply *reply)
{
    qDebug() << "replyFinished()";
    if(reply->error()) {
        qDebug() << "Downloader -- ERROR:" << reply->errorString();
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
