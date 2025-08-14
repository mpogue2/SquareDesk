#ifndef EMBEDDEDSERVER_H
#define EMBEDDEDSERVER_H

#include <QObject>
#ifdef Q_OS_LINUX
#include <QtHttpServer/QHttpServer>
#include <QtHttpServer/QHttpServerResponse>
#include <QtHttpServer/QHttpServerRequest>
#include <QTcpServer>
#else
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QHttpServerRequest>
#include <QTcpServer>
#endif
#include <QMimeDatabase>

class EmbeddedServer : public QObject
{
    Q_OBJECT

public:
    explicit EmbeddedServer(QObject *parent = nullptr);
    ~EmbeddedServer();
    
    bool start();
    void stop();
    QString getServerUrl() const;
    bool isRunning() const;

private:
    void setupRoutes();
    QHttpServerResponse serveFile(const QString &filePath);
    QHttpServerResponse serveNotFound();
    QHttpServerResponse serveUnauthorized();
    QHttpServerResponse handleRequest(const QHttpServerRequest &request);
    QString getMimeType(const QString &fileName);
    bool isValidRequest(const QHttpServerRequest &request);
    QString generateSecretToken();
    
    QHttpServer *m_server;
    QTcpServer *m_tcpServer;
    QString m_webRoot;
    quint16 m_port;
    QString m_secretToken;
    QMimeDatabase m_mimeDb;
    
    static constexpr quint16 MIN_PORT = 8000;
    static constexpr quint16 MAX_PORT = 8999;
};

#endif // EMBEDDEDSERVER_H
