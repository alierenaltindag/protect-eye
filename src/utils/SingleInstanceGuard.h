#pragma once

#include <QObject>
#include <QLockFile>
#include <QLocalServer>
#include <memory>

class SingleInstanceGuard : public QObject {
    Q_OBJECT

public:
    explicit SingleInstanceGuard(QObject* parent = nullptr);
    ~SingleInstanceGuard() override;

    bool tryRun();

signals:
    void activateRequested();

private slots:
    void onNewConnection();

private:
    QString userIdentifier() const;

    std::unique_ptr<QLockFile> m_lockFile;
    std::unique_ptr<QLocalServer> m_localServer;
    QString m_serverName;
};
