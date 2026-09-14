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
    if (checkWindowsDnd()) {
        return true;
    }
    if (checkWindowsFullscreen()) {
        return true;
    }
    return false;
}

bool DndMonitor::checkWindowsDnd() const {
    QUERY_USER_NOTIFICATION_STATE state;
    if (SHQueryUserNotificationState(&state) == S_OK) {
        int val = static_cast<int>(state);
        // 2: QUNS_BUSY (PowerPoint presentation, full screen application)
        // 3: QUNS_RUNNING_D3D_FULL_SCREEN (Direct3D exclusive game)
        // 4: QUNS_PRESENTATION_MODE (Windows Presentation Mode)
        // 6: QUNS_QUIET_HOURS (Windows 10/11 Focus Assist / DND)
        // 7: QUNS_APP (Focus Assist Priority / Alarms Only)
        if (val == 2 || val == 3 || val == 4 || val == 6 || val == 7) {
            return true;
        }
    }
    return false;
}

bool DndMonitor::checkWindowsFullscreen() const {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd || hwnd == GetDesktopWindow() || hwnd == GetShellWindow()) {
        return false;
    }

    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    if (!(style & WS_VISIBLE)) {
        return false;
    }

    wchar_t className[64] = {0};
    if (GetClassNameW(hwnd, className, 64) > 0) {
        if (wcscmp(className, L"Shell_TrayWnd") == 0 ||
            wcscmp(className, L"Progman") == 0 ||
            wcscmp(className, L"WorkerW") == 0) {
            return false;
        }
    }

    RECT appRect;
    if (!GetWindowRect(hwnd, &appRect)) {
        return false;
    }

    HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    if (!hMon) return false;

    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    if (!GetMonitorInfoW(hMon, &mi)) return false;

    if (appRect.left <= mi.rcMonitor.left &&
        appRect.top <= mi.rcMonitor.top &&
        appRect.right >= mi.rcMonitor.right &&
        appRect.bottom >= mi.rcMonitor.bottom) {
        return true;
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
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 150);
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
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 150);
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
    if (checkX11Fullscreen() || checkWaylandFullscreen()) {
        return true;
    }

    // 2. Desktop Environment specific checks
    if (m_desktopEnvironment.contains("GNOME") || m_desktopEnvironment.contains("UBUNTU")) {
        if (checkGnomeDnd()) return true;
    } else if (m_desktopEnvironment.contains("KDE")) {
        if (checkKdeDnd()) return true;
    } else if (m_desktopEnvironment.contains("XFCE")) {
        if (checkXfceDnd()) return true;
    } else if (m_desktopEnvironment.contains("CINNAMON") || m_desktopEnvironment.contains("X-CINNAMON")) {
        if (checkCinnamonDnd()) return true;
    } else if (m_desktopEnvironment.contains("MATE")) {
        if (checkMateDnd()) return true;
    } else if (m_desktopEnvironment.contains("COSMIC")) {
        if (checkFreedesktopInhibited()) return true;
    } else if (m_desktopEnvironment.contains("SWAY") || m_desktopEnvironment.contains("HYPRLAND")) {
        if (checkSwayNcDnd() || checkDunstDnd() || checkMakoDnd() || checkFreedesktopInhibited()) {
            return true;
        }
    }

    // 3. Generic fallback across all environments: FreeDesktop standard -> standalone tools -> DEs
    if (checkFreedesktopInhibited()) return true;
    if (checkSwayNcDnd()) return true;
    if (checkDunstDnd()) return true;
    if (checkMakoDnd()) return true;
    if (checkKdeDnd()) return true;
    if (checkGnomeDnd()) return true;
    if (checkXfceDnd()) return true;
    if (checkCinnamonDnd()) return true;
    if (checkMateDnd()) return true;

    return false;
}

bool DndMonitor::checkXfceDnd() const {
    if (checkFreedesktopInhibited()) {
        return true;
    }
    QProcess proc;
    proc.start("xfconf-query", QStringList() << "-c" << "xfce4-notifyd" << "-p" << "/do-not-disturb");
    if (proc.waitForFinished(200)) {
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
    if (proc.waitForFinished(200)) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (output == "false") {
            return true;
        }
    }
    // Fallback to dconf command if gsettings is unavailable or timed out
    QProcess dconfProc;
    dconfProc.start("dconf", QStringList() << "read" << "/org/gnome/desktop/notifications/show-banners");
    if (dconfProc.waitForFinished(200)) {
        QString output = QString::fromUtf8(dconfProc.readAllStandardOutput()).trimmed();
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
    if (proc.waitForFinished(200)) {
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
    if (proc.waitForFinished(200)) {
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
    if (queryDbusPropertyBool(QStringLiteral("org.erikreider.swaync"),
                              QStringLiteral("/org/erikreider/swaync"),
                              QStringLiteral("org.erikreider.swaync"),
                              QStringLiteral("dnd"))) {
        return true;
    }
    QProcess proc;
    proc.start("swaync-client", QStringList() << "-D");
    if (proc.waitForFinished(150)) {
        QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed().toLower();
        if (out == "true") {
            return true;
        }
    }
    return false;
}

bool DndMonitor::checkDunstDnd() const {
    if (queryDbusMethodBool(QStringLiteral("org.freedesktop.Notifications"),
                            QStringLiteral("/org/freedesktop/Notifications"),
                            QStringLiteral("org.dunstproject.cmd0"),
                            QStringLiteral("isPaused"))) {
        return true;
    }
    QProcess proc;
    proc.start("dunstctl", QStringList() << "is-paused");
    if (proc.waitForFinished(150)) {
        QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed().toLower();
        if (out == "true") {
            return true;
        }
    }
    return false;
}

bool DndMonitor::checkMakoDnd() const {
    QProcess proc;
    proc.start("makoctl", QStringList() << "mode");
    if (proc.waitForFinished(150)) {
        QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed().toLower();
        if (out.contains("dnd")) {
            return true;
        }
    }
    return false;
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
    if (queryDbusPropertyBool(QStringLiteral("org.freedesktop.Notifications"),
                              QStringLiteral("/org/freedesktop/Notifications"),
                              QStringLiteral("org.freedesktop.Notifications"),
                              QStringLiteral("inhibited"))) {
        return true;
    }
    if (queryDbusMethodBool(QStringLiteral("org.freedesktop.Notifications"),
                            QStringLiteral("/org/freedesktop/Notifications"),
                            QStringLiteral("org.freedesktop.Notifications"),
                            QStringLiteral("IsInhibited"))) {
        return true;
    }
    return false;
}

bool DndMonitor::checkX11Fullscreen() const {
    if (qEnvironmentVariableIsEmpty("DISPLAY")) {
        return false;
    }
    // Check if the foreground active window is in Fullscreen mode on X11 / XWayland
    QProcess proc;
    proc.start("xprop", QStringList() << "-root" << "_NET_ACTIVE_WINDOW");
    if (proc.waitForFinished(120)) {
        QString out = QString::fromUtf8(proc.readAllStandardOutput());
        int hashIdx = out.indexOf('#');
        if (hashIdx >= 0) {
            QString winId = out.mid(hashIdx + 1).trimmed();
            if (!winId.isEmpty() && winId != QStringLiteral("0x0")) {
                QProcess stateProc;
                stateProc.start("xprop", QStringList() << "-id" << winId << "_NET_WM_STATE");
                if (stateProc.waitForFinished(120)) {
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

bool DndMonitor::checkWaylandFullscreen() const {
    // 1. Check Hyprland
    if (!qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE")) {
        QProcess proc;
        proc.start("hyprctl", QStringList() << "activewindow" << "-j");
        if (proc.waitForFinished(150)) {
            QByteArray out = proc.readAllStandardOutput();
            if (out.contains("\"fullscreen\": true") ||
                out.contains("\"fullscreen\":true") ||
                out.contains("\"fullscreenMode\": 1") ||
                out.contains("\"fullscreenMode\": 2")) {
                return true;
            }
        }
    }

    // 2. Check Sway
    if (!qEnvironmentVariableIsEmpty("SWAYSOCK")) {
        QProcess proc;
        proc.start("swaymsg", QStringList() << "-t" << "get_focused");
        if (proc.waitForFinished(150)) {
            QByteArray out = proc.readAllStandardOutput();
            if (out.contains("\"fullscreen_mode\": 1") || out.contains("\"fullscreen_mode\":1")) {
                return true;
            }
        }
    }

    return false;
}
#endif
