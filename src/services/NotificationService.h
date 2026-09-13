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

    void setTrayIcon(QSystemTrayIcon* trayIcon) { m_trayIcon = trayIcon; }

    void showPreBreakWarning(bool isLongBreak, int secondsLeft);
    void showCustomNotification(const QString& title, const QString& message);

private:
    bool sendDbusNotification(const QString& title, const QString& message, int timeoutMs = 5000);

    QPointer<QSystemTrayIcon> m_trayIcon;
};
