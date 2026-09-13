#pragma once

#include <QObject>
#include <QString>
#include <QPointer>
#include <QSystemTrayIcon>

class NotificationService : public QObject {
    Q_OBJECT

public:
    explicit NotificationService(QSystemTrayIcon* trayIcon = nullptr, QObject* parent = nullptr);
    ~NotificationService() override = default;

    void setTrayIcon(QSystemTrayIcon* trayIcon);

    void showPreBreakWarning(bool isLongBreak, int secondsLeft);
    void showCustomNotification(const QString& title, const QString& message);

public slots:
    void showUpdateNotification(const QString& version, const QString& releaseUrl);

private slots:
    void onTrayMessageClicked();
#ifndef Q_OS_WIN
    void onDbusActionInvoked(uint id, const QString& actionKey);
#endif

private:
    bool sendDbusNotification(const QString& title, const QString& message, int timeoutMs = 5000, const QStringList& actions = QStringList());

    QPointer<QSystemTrayIcon> m_trayIcon;
    QString m_pendingReleaseUrl;
    uint m_lastUpdateNotificationId{0};
};
