#include "NotificationService.h"
#include "core/Localization.h"
#include <QStringList>

#ifndef Q_OS_WIN
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusConnection>
#include <QVariantMap>
#endif

NotificationService::NotificationService(QSystemTrayIcon* trayIcon, QObject* parent)
    : QObject(parent)
    , m_trayIcon(trayIcon) {
}

void NotificationService::showPreBreakWarning(bool isLongBreak, int secondsLeft) {
    auto& loc = Localization::instance();
    QString title = loc.preBreakTitle(isLongBreak);
    QString message = loc.preBreakMessage(secondsLeft);
    showCustomNotification(title, message);
}


void NotificationService::showCustomNotification(const QString& title, const QString& message) {
#ifndef Q_OS_WIN
    if (sendDbusNotification(title, message, 6000)) {
        return;
    }
#endif
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 5000);
    }
}

bool NotificationService::sendDbusNotification(const QString& title, const QString& message, int timeoutMs) {
#ifdef Q_OS_WIN
    Q_UNUSED(title);
    Q_UNUSED(message);
    Q_UNUSED(timeoutMs);
    return false;
#else
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("Notify")
    );

    QString appName = QStringLiteral("ProtectEye");
    uint replacesId = 0;
    QString appIcon = QStringLiteral("protecteye");
    QStringList actions;
    QVariantMap hints;
    hints[QStringLiteral("urgency")] = uchar(1);
    hints[QStringLiteral("desktop-entry")] = QStringLiteral("protecteye");

    msg << appName << replacesId << appIcon << title << message << actions << hints << timeoutMs;

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 1000);
    return (reply.type() == QDBusMessage::ReplyMessage);
#endif
}
