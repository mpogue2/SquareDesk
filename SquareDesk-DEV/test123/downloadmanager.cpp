/****************************************************************************
**
** Copyright (C) 2016-2022 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
**
** $SQUAREDESK_BEGIN_LICENSE$
**
** Commercial License Usage
** For commercial licensing terms and conditions, contact the authors via the
** email address above.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appear in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.
**
** $SQUAREDESK_END_LICENSE$
**
****************************************************************************/

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
