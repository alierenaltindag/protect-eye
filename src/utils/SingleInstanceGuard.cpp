#include "SingleInstanceGuard.h"
#include <QDir>
#include <QLocalSocket>
#include <QCryptographicHash>
#include <QProcessEnvironment>
#include <QDebug>

SingleInstanceGuard::SingleInstanceGuard(QObject* parent)
    : QObject(parent) {
    const QString userHash = userIdentifier();
    const QString lockPath = QDir::temp().filePath(QStringLiteral("protecteye_%1.lock").arg(userHash));
    m_lockFile = std::make_unique<QLockFile>(lockPath);
    m_lockFile->setStaleLockTime(0); // 0 = check if process holding the lock is still alive immediately

    m_serverName = QStringLiteral("protecteye_ipc_%1").arg(userHash);
}

SingleInstanceGuard::~SingleInstanceGuard() {
    if (m_localServer) {
        m_localServer->close();
        QLocalServer::removeServer(m_serverName);
    }
    if (m_lockFile && m_lockFile->isLocked()) {
        m_lockFile->unlock();
    }
}

QString SingleInstanceGuard::userIdentifier() const {
    const auto env = QProcessEnvironment::systemEnvironment();
    QString user = env.value(QStringLiteral("USER"));
    if (user.isEmpty()) {
        user = env.value(QStringLiteral("USERNAME"));
    }
    if (user.isEmpty()) {
        user = QStringLiteral("default_user");
    }
    const QByteArray hash = QCryptographicHash::hash(user.toUtf8(), QCryptographicHash::Md5).toHex();
    return QString::fromUtf8(hash.left(8));
}

bool SingleInstanceGuard::tryRun() {
    if (!m_lockFile->tryLock(200)) {
        // Another instance is holding the lock. Send ACTIVATE to notify primary instance.
        QLocalSocket socket;
        socket.connectToServer(m_serverName);
        if (socket.waitForConnected(400)) {
            socket.write("ACTIVATE\n");
            socket.waitForBytesWritten(400);
            socket.disconnectFromServer();
        }
        return false;
    }

    // We are the primary instance. Clean up any stale socket and start listening.
    QLocalServer::removeServer(m_serverName);
    m_localServer = std::make_unique<QLocalServer>(this);
    connect(m_localServer.get(), &QLocalServer::newConnection, this, &SingleInstanceGuard::onNewConnection);

    if (!m_localServer->listen(m_serverName)) {
        qWarning() << "SingleInstanceGuard: could not start local server on" << m_serverName;
    }

    return true;
}

void SingleInstanceGuard::onNewConnection() {
    while (m_localServer && m_localServer->hasPendingConnections()) {
        QLocalSocket* client = m_localServer->nextPendingConnection();
        if (!client) continue;

        connect(client, &QLocalSocket::readyRead, this, [this, client]() {
            const QByteArray data = client->readAll();
            if (data.contains("ACTIVATE")) {
                emit activateRequested();
            }
        });

        connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
    }
}
