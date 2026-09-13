#include "NotificationService.h"
#include "core/Localization.h"
#include <QStringList>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

#ifndef Q_OS_WIN
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusConnection>
#include <QVariantMap>
#endif

NotificationService::NotificationService(QSystemTrayIcon* trayIcon, QObject* parent)
    : QObject(parent)
    , m_trayIcon(trayIcon) {
    if (m_trayIcon) {
        connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &NotificationService::onTrayMessageClicked);
    }

#ifndef Q_OS_WIN
    QDBusConnection::sessionBus().connect(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("ActionInvoked"),
        this,
        SLOT(onDbusActionInvoked(uint, QString))
    );
#endif
}

void NotificationService::setTrayIcon(QSystemTrayIcon* trayIcon) {
    if (m_trayIcon) {
        disconnect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &NotificationService::onTrayMessageClicked);
    }
    m_trayIcon = trayIcon;
    if (m_trayIcon) {
        connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &NotificationService::onTrayMessageClicked);
    }
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

void NotificationService::showUpdateNotification(const QString& version, const QString& releaseUrl) {
    m_pendingReleaseUrl = releaseUrl.isEmpty()
        ? QStringLiteral("https://github.com/alierenaltindag/protect-eye/releases/latest")
        : releaseUrl;

    auto& loc = Localization::instance();
    QString title = loc.updateNotifTitle();
    QString message = loc.updateNotifBody().arg(version);

#ifndef Q_OS_WIN
    // Send native desktop manager notification via DBus
    QStringList actions;
    actions << QStringLiteral("default") << loc.actionUpdateAvailable().arg(version);
    if (sendDbusNotification(title, message, 10000, actions)) {
        return;
    }
#endif

    // Windows Action Center / Toast or tray fallback
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 10000);
    }
}

void NotificationService::onTrayMessageClicked() {
    if (!m_pendingReleaseUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_pendingReleaseUrl));
    }
}

#ifndef Q_OS_WIN
void NotificationService::onDbusActionInvoked(uint id, const QString& actionKey) {
    Q_UNUSED(actionKey);
    if (id == m_lastUpdateNotificationId && !m_pendingReleaseUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_pendingReleaseUrl));
    }
}
#endif

bool NotificationService::sendDbusNotification(const QString& title, const QString& message, int timeoutMs, const QStringList& actions) {
#ifdef Q_OS_WIN
    Q_UNUSED(title);
    Q_UNUSED(message);
    Q_UNUSED(timeoutMs);
    Q_UNUSED(actions);
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
    QVariantMap hints;
    hints[QStringLiteral("urgency")] = uchar(1);
    hints[QStringLiteral("desktop-entry")] = QStringLiteral("protecteye");

    msg << appName << replacesId << appIcon << title << message << actions << hints << timeoutMs;

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 1000);
    if (reply.type() == QDBusMessage::ReplyMessage) {
        if (!reply.arguments().isEmpty()) {
            m_lastUpdateNotificationId = reply.arguments().at(0).toUInt();
        }
        return true;
    }
    return false;
#endif
}
