#include "embeddedserver.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QHostAddress>
#include <QHttpHeaders>
#include <QUrl>
#include <QDebug>

EmbeddedServer::EmbeddedServer(QObject *parent)
    : QObject(parent)
    , m_server(nullptr)
    , m_tcpServer(nullptr)
    , m_port(0)
{
    // Set up the web root path
    QString appPath = QCoreApplication::applicationDirPath();
    m_webRoot = appPath + "/../Resources/Taminations/web";
    
    // Generate a secret token for security
    m_secretToken = generateSecretToken();
    
    // qDebug() << "Web root set to:" << m_webRoot;
    // qDebug() << "Secret token generated for session";
}

EmbeddedServer::~EmbeddedServer()
{
    stop();
}

bool EmbeddedServer::isRunning() const
{
    return m_tcpServer && m_tcpServer->isListening();
}

bool EmbeddedServer::start()
{
    if (m_server && m_tcpServer) {
        qWarning() << "Server is already running";
        return false;
    }
    
    // Verify web root exists
    QDir webDir(m_webRoot);
    if (!webDir.exists()) {
        qCritical() << "Web root directory does not exist:" << m_webRoot;
        return false;
    }
    
    // Check if index.html exists
    if (!QFile::exists(m_webRoot + "/index.html")) {
        qCritical() << "index.html not found in web root:" << m_webRoot;
        return false;
    }
    
    m_server = new QHttpServer(this);
    m_tcpServer = new QTcpServer(this);
    
    setupRoutes();
    
    // Try to find an available port
    for (quint16 port = MIN_PORT; port <= MAX_PORT; ++port) {
        // Only bind to localhost for security
        if (m_tcpServer->listen(QHostAddress::LocalHost, port)) {
            if (m_server->bind(m_tcpServer)) {
                m_port = port;
                // qInfo() << "Server started on http://localhost:" << m_port;
                // qInfo() << "Secret token:" << m_secretToken;
                return true;
            } else {
                m_tcpServer->close();
            }
        }
    }
    
    qCritical() << "Failed to start server on any port between" << MIN_PORT << "and" << MAX_PORT;
    delete m_server;
    delete m_tcpServer;
    m_server = nullptr;
    m_tcpServer = nullptr;
    return false;
}

void EmbeddedServer::stop()
{
    if (m_tcpServer) {
        m_tcpServer->close();
        delete m_tcpServer;
        m_tcpServer = nullptr;
    }
    if (m_server) {
        delete m_server;
        m_server = nullptr;
    }
    m_port = 0;
    qInfo() << "Server stopped";
}

QString EmbeddedServer::getServerUrl() const
{
    if (!m_tcpServer || !m_tcpServer->isListening()) {
        return QString();
    }
    // return QString("http://localhost:%1?token=%2").arg(m_port).arg(m_secretToken); // WITH TOKEN
    return QString("http://localhost:%1").arg(m_port); // NO TOKEN
}

void EmbeddedServer::setupRoutes()
{
    // Route for root path
    m_server->route("/", [this](const QHttpServerRequest &request) {
        return handleRequest(request);
    });
    
    // Route for any path with single parameter
    m_server->route("/<arg>", [this](const QString &path, const QHttpServerRequest &request) {
        Q_UNUSED(path);
        return handleRequest(request);
    });
    
    // Route for paths with multiple segments (like /js/app.js, /css/style.css, etc.)
    m_server->route("/<arg>/<arg>", [this](const QString &path1, const QString &path2, const QHttpServerRequest &request) {
        Q_UNUSED(path1);
        Q_UNUSED(path2);
        return handleRequest(request);
    });
    
    // Route for even deeper paths (3 segments)
    m_server->route("/<arg>/<arg>/<arg>", [this](const QString &path1, const QString &path2, const QString &path3, const QHttpServerRequest &request) {
        Q_UNUSED(path1);
        Q_UNUSED(path2);
        Q_UNUSED(path3);
        return handleRequest(request);
    });
    
    // Route for 4 segments (like assets/assets/info/about.md)
    m_server->route("/<arg>/<arg>/<arg>/<arg>", [this](const QString &path1, const QString &path2, const QString &path3, const QString &path4, const QHttpServerRequest &request) {
        Q_UNUSED(path1);
        Q_UNUSED(path2);
        Q_UNUSED(path3);
        Q_UNUSED(path4);
        return handleRequest(request);
    });
    
    // Route for 5 segments (just in case)
    m_server->route("/<arg>/<arg>/<arg>/<arg>/<arg>", [this](const QString &path1, const QString &path2, const QString &path3, const QString &path4, const QString &path5, const QHttpServerRequest &request) {
        Q_UNUSED(path1);
        Q_UNUSED(path2);
        Q_UNUSED(path3);
        Q_UNUSED(path4);
        Q_UNUSED(path5);
        return handleRequest(request);
    });
}

QHttpServerResponse EmbeddedServer::handleRequest(const QHttpServerRequest &request)
{
    // Security check: validate the request
    if (!isValidRequest(request)) {
        qWarning() << "Unauthorized request from:" << request.remoteAddress().toString();
        return serveUnauthorized();
    }
    
    QString path = request.url().path();
    // qDebug() << "Request for path:" << path;  // Debug all requests
    
    // Handle root requests with query parameters
    if (path == "/" || path.isEmpty()) {
        QString queryString = request.url().query();
        if (!queryString.isEmpty()) {
            // Check if this is already a redirected request to avoid infinite loop
            // Look for our redirect marker or check if we're already serving the right content
            if (!queryString.contains("_redirected=1")) {
                // If there are query parameters and we haven't redirected yet,
                // redirect to preserve them in the main URL with a marker to prevent loops
                QString redirectUrl = QString("http://localhost:%1/?%2&_redirected=1#/").arg(m_port).arg(queryString);
                
                QHttpServerResponse response(QHttpServerResponder::StatusCode::Found);
                QHttpHeaders headers;
                headers.append("Location", redirectUrl.toUtf8());
                response.setHeaders(headers);
                
                // qDebug() << "Redirecting to preserve query params:" << redirectUrl;
                // qDebug() << "Original query string was:" << queryString;
                return response;
            }
        }
        path = "/index.html";
    }
    
    // Security: prevent directory traversal - check for dangerous patterns first
    if (path.contains("..") || path.contains("\\")) {
        qWarning() << "Directory traversal attempt blocked (dangerous pattern):" << path;
        return serveNotFound();
    }
    
    QString filePath = m_webRoot + path;
    
    // Security: normalize and validate the path
    QDir webDir(m_webRoot);
    QString canonicalWebRoot = webDir.canonicalPath();
    
    // For existing files, use canonical path
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        QString canonicalPath = fileInfo.canonicalFilePath();
        if (!canonicalPath.startsWith(canonicalWebRoot)) {
            qWarning() << "Directory traversal attempt blocked (canonical path):" << path;
            return serveNotFound();
        }
    } else {
        // For non-existing files, check if the directory path would be valid
        QString dirPath = fileInfo.absolutePath();
        QFileInfo dirInfo(dirPath);
        if (dirInfo.exists()) {
            QString canonicalDirPath = dirInfo.canonicalFilePath();
            if (!canonicalDirPath.startsWith(canonicalWebRoot)) {
                qWarning() << "Directory traversal attempt blocked (directory path):" << path;
                return serveNotFound();
            }
        }
        // If directory doesn't exist either, let serveFile handle the 404
    }
    
    return serveFile(filePath);
}

QHttpServerResponse EmbeddedServer::serveFile(const QString &filePath)
{
    // qDebug() << "Attempting to serve file:" << filePath;
    
    QFile file(filePath);
    
    if (!file.exists()) {
        qWarning() << "File not found:" << filePath;
        
        // Debug: List what files ARE in the directory
        QFileInfo fileInfo(filePath);
        QString dirPath = fileInfo.absolutePath();
        QDir dir(dirPath);
        if (dir.exists()) {
            // qDebug() << "Directory" << dirPath << "contents:";
            QStringList files = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString &fileName : files) {
                qDebug() << "  " << fileName;
            }
        } else {
            qDebug() << "Directory does not exist:" << dirPath;
        }
        
        return serveNotFound();
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file:" << filePath;
        return QHttpServerResponse(QHttpServerResponder::StatusCode::InternalServerError);
    }
    
    QByteArray content = file.readAll();
    QString mimeType = getMimeType(filePath);
    
    QHttpServerResponse response(content, QHttpServerResponder::StatusCode::Ok);
    
    // Set headers using the proper Qt 6.8 API
    QHttpHeaders headers;
    headers.append(QHttpHeaders::WellKnownHeader::ContentType, mimeType.toUtf8());
    headers.append(QHttpHeaders::WellKnownHeader::CacheControl, "no-cache, no-store, must-revalidate");
    headers.append("Pragma", "no-cache");
    headers.append("Expires", "0");
    headers.append("Access-Control-Allow-Origin", "*");
    headers.append("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    headers.append("Access-Control-Allow-Headers", "Content-Type");
    
    response.setHeaders(headers);
    
    return response;
}

QHttpServerResponse EmbeddedServer::serveNotFound()
{
    QString html = "<html><body><h1>404 - Not Found</h1></body></html>";
    return QHttpServerResponse(html.toUtf8(), QHttpServerResponder::StatusCode::NotFound);
}

QHttpServerResponse EmbeddedServer::serveUnauthorized()
{
    QString html = "<html><body><h1>401 - Unauthorized</h1></body></html>";
    return QHttpServerResponse(html.toUtf8(), QHttpServerResponder::StatusCode::Unauthorized);
}

QString EmbeddedServer::getMimeType(const QString &fileName)
{
    QMimeType mimeType = m_mimeDb.mimeTypeForFile(fileName);
    return mimeType.name();
}

bool EmbeddedServer::isValidRequest(const QHttpServerRequest &request)
{
    // Check 1: Must be from localhost
    QHostAddress remoteAddr = request.remoteAddress();
    if (remoteAddr != QHostAddress::LocalHost && 
        remoteAddr != QHostAddress("127.0.0.1") &&
        remoteAddr != QHostAddress("::1")) {
        return false;
    }
    
    // Check 2: Must have valid token (for first request) or valid Referer
    // QUrl url = request.url();
    // QString queryString = url.query(QUrl::FullyDecoded);
    // QString referer = request.value("Referer");
    
    // // Allow if token matches
    // if (queryString.contains(QString("token=%1").arg(m_secretToken))) {
    //     return true;
    // }

    return true;
    
    // // Allow if referer is from our server
    // if (!referer.isEmpty()) {
    //     QUrl refererUrl(referer);
    //     if (refererUrl.host() == "localhost" &&
    //         refererUrl.port() == m_port) {
    //         return true;
    //     }
    // }
    
    // return false;
}

QString EmbeddedServer::generateSecretToken()
{
    QRandomGenerator *rng = QRandomGenerator::global();
    QString token;
    
    for (int i = 0; i < 32; ++i) {
        token += QString::number(rng->bounded(16), 16);
    }
    
    return token;
}
