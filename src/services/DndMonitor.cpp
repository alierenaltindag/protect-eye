#include "DndMonitor.h"
#include <cstdlib>

#ifdef Q_OS_WIN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WINVER
#define WINVER 0x0A00
#endif
#include <windows.h>
#include <shellapi.h>

DndMonitor::DndMonitor(QObject* parent)
    : QObject(parent) {
}

bool DndMonitor::isDndActive() const {
    return checkWindowsDnd();
}

bool DndMonitor::checkWindowsDnd() const {
    QUERY_USER_NOTIFICATION_STATE state;
    if (SHQueryUserNotificationState(&state) == S_OK) {
        int val = static_cast<int>(state);
        // 2: QUNS_BUSY
        // 3: QUNS_RUNNING_D3D_FULL_SCREEN
        // 4: QUNS_PRESENTATION_MODE
        // 6: QUNS_QUIET_HOURS (Windows 10/11 Focus Assist / DND)
        // 7: QUNS_APP (Focus Assist Alarms / Priority Only)
        if (val == 2 || val == 3 || val == 4 || val == 6 || val == 7) {
            return true;
        }
    }
    return false;
}

#else

#include <QProcess>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusVariant>
#include <QDebug>

namespace {

bool queryDbusPropertyBool(const QString& service, const QString& path, const QString& interface, const QString& property) {
    QDBusMessage msg = QDBusMessage::createMethodCall(
        service, path, QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get")
    );
    msg << interface << property;
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 200);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        QVariant v = reply.arguments().at(0);
        if (v.canConvert<QDBusVariant>()) {
            return v.value<QDBusVariant>().variant().toBool();
        }
        return v.toBool();
    }
    return false;
}

bool queryDbusMethodBool(const QString& service, const QString& path, const QString& interface, const QString& method) {
    QDBusMessage msg = QDBusMessage::createMethodCall(service, path, interface, method);
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 200);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        return reply.arguments().at(0).toBool();
    }
    return false;
}

} // namespace

DndMonitor::DndMonitor(QObject* parent)
    : QObject(parent) {
    const char* xdgDesktop = std::getenv("XDG_CURRENT_DESKTOP");
    if (xdgDesktop) {
        m_desktopEnvironment = QString::fromUtf8(xdgDesktop).toUpper();
    }
}

bool DndMonitor::isDndActive() const {
    // 1. Check if an active foreground window is currently Fullscreen (gaming, video, presentation)
    if (checkX11Fullscreen()) {
        return true;
    }

    if (m_desktopEnvironment.contains("GNOME") || m_desktopEnvironment.contains("UBUNTU")) {
        return checkGnomeDnd();
    } else if (m_desktopEnvironment.contains("KDE")) {
        return checkKdeDnd();
    } else if (m_desktopEnvironment.contains("XFCE")) {
        return checkXfceDnd();
    } else if (m_desktopEnvironment.contains("CINNAMON") || m_desktopEnvironment.contains("X-CINNAMON")) {
        return checkCinnamonDnd();
    } else if (m_desktopEnvironment.contains("MATE")) {
        return checkMateDnd();
    } else if (m_desktopEnvironment.contains("COSMIC")) {
        // System76 COSMIC desktop implements standard FreeDesktop Notifications Inhibited
        return checkFreedesktopInhibited();
    } else if (m_desktopEnvironment.contains("SWAY") || m_desktopEnvironment.contains("HYPRLAND")) {
        if (checkSwayNcDnd() || checkDunstDnd() || checkFreedesktopInhibited()) {
            return true;
        }
    }

    // Generic fallback across all environments: FreeDesktop standard -> standalone tools -> DEs
    enum FallbackMethod {
        FdoInhibited = 0,
        SwayNc = 1,
        Dunst = 2,
        Kde = 3,
        Gnome = 4,
        Xfce = 5,
        Cinnamon = 6,
        Mate = 7
    };

    if (m_cachedFallbackMethod >= 0) {
        switch (m_cachedFallbackMethod) {
        case FdoInhibited: return checkFreedesktopInhibited();
        case SwayNc: return checkSwayNcDnd();
        case Dunst: return checkDunstDnd();
        case Kde: return checkKdeDnd();
        case Gnome: return checkGnomeDnd();
        case Xfce: return checkXfceDnd();
        case Cinnamon: return checkCinnamonDnd();
        case Mate: return checkMateDnd();
        default: break;
        }
    }

    if (checkFreedesktopInhibited()) { m_cachedFallbackMethod = FdoInhibited; return true; }
    if (checkSwayNcDnd()) { m_cachedFallbackMethod = SwayNc; return true; }
    if (checkDunstDnd()) { m_cachedFallbackMethod = Dunst; return true; }
    if (checkKdeDnd()) { m_cachedFallbackMethod = Kde; return true; }
    if (checkGnomeDnd()) { m_cachedFallbackMethod = Gnome; return true; }
    if (checkXfceDnd()) { m_cachedFallbackMethod = Xfce; return true; }
    if (checkCinnamonDnd()) { m_cachedFallbackMethod = Cinnamon; return true; }
    if (checkMateDnd()) { m_cachedFallbackMethod = Mate; return true; }

    return false;
}

bool DndMonitor::checkXfceDnd() const {
    if (checkFreedesktopInhibited()) {
        return true;
    }
    QProcess proc;
    proc.start("xfconf-query", QStringList() << "-c" << "xfce4-notifyd" << "-p" << "/do-not-disturb");
    if (proc.waitForFinished(100)) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed().toLower();
        if (output == "true") {
            return true;
        }
    }
    return false;
}

bool DndMonitor::checkGnomeDnd() const {
    if (checkFreedesktopInhibited()) {
        return true;
    }
    QProcess proc;
    proc.start("gsettings", QStringList() << "get" << "org.gnome.desktop.notifications" << "show-banners");
    if (proc.waitForFinished(100)) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (output == "false") {
            return true;
        }
    }
    return false;
}

bool DndMonitor::checkCinnamonDnd() const {
    if (checkFreedesktopInhibited()) {
        return true;
    }
    QProcess proc;
    proc.start("gsettings", QStringList() << "get" << "org.cinnamon.desktop.notifications" << "display-notifications");
    if (proc.waitForFinished(100)) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (output == "false") {
            return true;
        }
    }
    return false;
}

bool DndMonitor::checkMateDnd() const {
    if (checkFreedesktopInhibited()) {
        return true;
    }
    QProcess proc;
    proc.start("gsettings", QStringList() << "get" << "org.mate.NotificationDaemon" << "do-not-disturb");
    if (proc.waitForFinished(100)) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (output == "true") {
            return true;
        }
    }
    return false;
}

bool DndMonitor::checkSwayNcDnd() const {
    if (queryDbusMethodBool(QStringLiteral("org.erikreider.swaync"),
                            QStringLiteral("/org/erikreider/swaync"),
                            QStringLiteral("org.erikreider.swaync"),
                            QStringLiteral("GetDnd"))) {
        return true;
    }
    return queryDbusPropertyBool(QStringLiteral("org.erikreider.swaync"),
                                 QStringLiteral("/org/erikreider/swaync"),
                                 QStringLiteral("org.erikreider.swaync"),
                                 QStringLiteral("dnd"));
}

bool DndMonitor::checkDunstDnd() const {
    return queryDbusMethodBool(QStringLiteral("org.freedesktop.Notifications"),
                               QStringLiteral("/org/freedesktop/Notifications"),
                               QStringLiteral("org.dunstproject.cmd0"),
                               QStringLiteral("isPaused"));
}

bool DndMonitor::checkKdeDnd() const {
    if (queryDbusPropertyBool(QStringLiteral("org.kde.plasma.notifications"),
                              QStringLiteral("/org/kde/plasma/notifications"),
                              QStringLiteral("org.kde.plasma.notifications"),
                              QStringLiteral("dnd"))) {
        return true;
    }
    if (queryDbusPropertyBool(QStringLiteral("org.kde.plasma.notifications"),
                              QStringLiteral("/org/kde/plasma/notifications"),
                              QStringLiteral("org.kde.plasma.notifications"),
                              QStringLiteral("inhibited"))) {
        return true;
    }
    return checkFreedesktopInhibited();
}

bool DndMonitor::checkFreedesktopInhibited() const {
    if (queryDbusPropertyBool(QStringLiteral("org.freedesktop.Notifications"),
                              QStringLiteral("/org/freedesktop/Notifications"),
                              QStringLiteral("org.freedesktop.Notifications"),
                              QStringLiteral("Inhibited"))) {
        return true;
    }
    return queryDbusPropertyBool(QStringLiteral("org.freedesktop.Notifications"),
                                 QStringLiteral("/org/freedesktop/Notifications"),
                                 QStringLiteral("org.freedesktop.Notifications"),
                                 QStringLiteral("inhibited"));
}

bool DndMonitor::checkX11Fullscreen() const {
    // Check if the foreground active window is in Fullscreen mode on X11 / XWayland
    QProcess proc;
    proc.start("xprop", QStringList() << "-root" << "_NET_ACTIVE_WINDOW");
    if (proc.waitForFinished(80)) {
        QString out = QString::fromUtf8(proc.readAllStandardOutput());
        int hashIdx = out.indexOf('#');
        if (hashIdx >= 0) {
            QString winId = out.mid(hashIdx + 1).trimmed();
            if (!winId.isEmpty() && winId != QStringLiteral("0x0")) {
                QProcess stateProc;
                stateProc.start("xprop", QStringList() << "-id" << winId << "_NET_WM_STATE");
                if (stateProc.waitForFinished(80)) {
                    QString stateOut = QString::fromUtf8(stateProc.readAllStandardOutput());
                    if (stateOut.contains(QStringLiteral("_NET_WM_STATE_FULLSCREEN"))) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}
#endif
