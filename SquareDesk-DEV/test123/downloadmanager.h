#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QDebug>
#include <QTemporaryFile>

class Downloader : public QObject
{
    Q_OBJECT

public:
    explicit Downloader(QObject *parent = 0);

signals:
    void downloadFinished();

public slots:
    void replyFinished (QNetworkReply *reply);
    void doDownload(QUrl url, QString destPath);
    void abortTransfer();

private:
    QNetworkAccessManager *manager;
    QString destPath;
    QNetworkReply *networkReply;
};

#endif // DOWNLOADER_H
