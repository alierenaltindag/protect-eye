#include "ScreenLockMonitor.h"
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wtsapi32.h>
#include <QCoreApplication>
#include <QWidget>

class ScreenLockMonitor::WinSessionEventFilter : public QAbstractNativeEventFilter {
public:
    explicit WinSessionEventFilter(ScreenLockMonitor* owner) : m_owner(owner) {}

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override {
        Q_UNUSED(eventType);
        Q_UNUSED(result);
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_WTSSESSION_CHANGE) {
            if (msg->wParam == WTS_SESSION_LOCK) {
                m_owner->m_isLocked = true;
                emit m_owner->lockStateChanged(true);
            } else if (msg->wParam == WTS_SESSION_UNLOCK) {
                m_owner->m_isLocked = false;
                emit m_owner->lockStateChanged(false);
            }
        }
        return false;
    }

private:
    ScreenLockMonitor* m_owner;
};

ScreenLockMonitor::ScreenLockMonitor(QObject* parent)
    : QObject(parent) {
    setupConnections();
}

ScreenLockMonitor::~ScreenLockMonitor() {
    if (m_msgWindow) {
        WTSUnRegisterSessionNotification(reinterpret_cast<HWND>(m_msgWindow->winId()));
        delete m_msgWindow;
    }
}

void ScreenLockMonitor::setupConnections() {
    m_msgWindow = new QWidget();
    m_msgWindow->setAttribute(Qt::WA_DontShowOnScreen);
    HWND hwnd = reinterpret_cast<HWND>(m_msgWindow->winId());
    WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_THIS_SESSION);

    m_eventFilter = std::make_unique<WinSessionEventFilter>(this);
    QCoreApplication::instance()->installNativeEventFilter(m_eventFilter.get());
}

void ScreenLockMonitor::queryInitialState() {
    // Windows session starts unlocked when app launches
    m_isLocked = false;
}

#else

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QCoreApplication>

namespace {

bool queryMethodBool(const QString& service, const QString& path, const QString& interface, const QString& method) {
    QDBusMessage msg = QDBusMessage::createMethodCall(service, path, interface, method);
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 200);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        return reply.arguments().at(0).toBool();
    }
    return false;
}

} // namespace

ScreenLockMonitor::ScreenLockMonitor(QObject* parent)
    : QObject(parent) {
    setupConnections();
    queryInitialState();
}

ScreenLockMonitor::~ScreenLockMonitor() = default;

void ScreenLockMonitor::setupConnections() {
    auto bus = QDBusConnection::sessionBus();

    // GNOME ScreenSaver
    bus.connect("org.gnome.ScreenSaver",
                "/org/gnome/ScreenSaver",
                "org.gnome.ScreenSaver",
                "ActiveChanged",
                this,
                SLOT(onGnomeActiveChanged(bool)));

    // FreeDesktop / KDE ScreenSaver
    bus.connect("org.freedesktop.ScreenSaver",
                "/org/freedesktop/ScreenSaver",
                "org.freedesktop.ScreenSaver",
                "ActiveChanged",
                this,
                SLOT(onFreedesktopActiveChanged(bool)));

    // XFCE ScreenSaver
    bus.connect("org.xfce.ScreenSaver",
                "/org/xfce/ScreenSaver",
                "org.xfce.ScreenSaver",
                "ActiveChanged",
                this,
                SLOT(onFreedesktopActiveChanged(bool)));

    // Cinnamon ScreenSaver
    bus.connect("org.cinnamon.ScreenSaver",
                "/org/cinnamon/ScreenSaver",
                "org.cinnamon.ScreenSaver",
                "ActiveChanged",
                this,
                SLOT(onFreedesktopActiveChanged(bool)));

    // MATE ScreenSaver
    bus.connect("org.mate.ScreenSaver",
                "/org/mate/ScreenSaver",
                "org.mate.ScreenSaver",
                "ActiveChanged",
                this,
                SLOT(onFreedesktopActiveChanged(bool)));

    // systemd-logind (System Bus) - PrepareForSleep (suspend/resume across all distros)
    auto sysBus = QDBusConnection::systemBus();
    sysBus.connect("org.freedesktop.login1",
                   "/org/freedesktop/login1",
                   "org.freedesktop.login1.Manager",
                   "PrepareForSleep",
                   this,
                   SLOT(onFreedesktopActiveChanged(bool)));

    // systemd-logind (System Bus) - Query session ID specific to current process
    m_sessionPath.clear();
    QDBusMessage sessionMsg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.login1"),
        QStringLiteral("/org/freedesktop/login1"),
        QStringLiteral("org.freedesktop.login1.Manager"),
        QStringLiteral("GetSessionByPID")
    );
    sessionMsg << static_cast<quint32>(QCoreApplication::applicationPid());
    QDBusMessage sessionReply = sysBus.call(sessionMsg, QDBus::Block, 500);
    if (sessionReply.type() == QDBusMessage::ReplyMessage && !sessionReply.arguments().isEmpty()) {
        m_sessionPath = sessionReply.arguments().at(0).value<QDBusObjectPath>().path();
    }
    if (m_sessionPath.isEmpty()) {
        m_sessionPath = QStringLiteral("/org/freedesktop/login1/session/auto");
    }

    // Connect to session-specific Lock / Unlock signals
    sysBus.connect(QStringLiteral("org.freedesktop.login1"),
                   m_sessionPath,
                   QStringLiteral("org.freedesktop.login1.Session"),
                   QStringLiteral("Lock"),
                   this,
                   SLOT(onLogindSessionLocked()));

    sysBus.connect(QStringLiteral("org.freedesktop.login1"),
                   m_sessionPath,
                   QStringLiteral("org.freedesktop.login1.Session"),
                   QStringLiteral("Unlock"),
                   this,
                   SLOT(onLogindSessionUnlocked()));
}

void ScreenLockMonitor::queryInitialState() {
    // 1. Check systemd-logind LockedHint on system bus using sessionPath
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.login1"),
        m_sessionPath.isEmpty() ? QStringLiteral("/org/freedesktop/login1/session/auto") : m_sessionPath,
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("Get")
    );
    msg << QStringLiteral("org.freedesktop.login1.Session") << QStringLiteral("LockedHint");
    QDBusMessage reply = QDBusConnection::systemBus().call(msg, QDBus::Block, 200);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        QVariant v = reply.arguments().at(0);
        if (v.canConvert<QDBusVariant>()) {
            if (v.value<QDBusVariant>().variant().toBool()) {
                m_isLocked = true;
                return;
            }
        }
    }

    // 2. Check GNOME
    if (queryMethodBool(QStringLiteral("org.gnome.ScreenSaver"),
                        QStringLiteral("/org/gnome/ScreenSaver"),
                        QStringLiteral("org.gnome.ScreenSaver"),
                        QStringLiteral("GetActive"))) {
        m_isLocked = true;
        return;
    }

    // 3. Check Cinnamon
    if (queryMethodBool(QStringLiteral("org.cinnamon.ScreenSaver"),
                        QStringLiteral("/org/cinnamon/ScreenSaver"),
                        QStringLiteral("org.cinnamon.ScreenSaver"),
                        QStringLiteral("GetActive"))) {
        m_isLocked = true;
        return;
    }

    // 4. Check MATE
    if (queryMethodBool(QStringLiteral("org.mate.ScreenSaver"),
                        QStringLiteral("/org/mate/ScreenSaver"),
                        QStringLiteral("org.mate.ScreenSaver"),
                        QStringLiteral("GetActive"))) {
        m_isLocked = true;
        return;
    }

    // 5. Check XFCE
    if (queryMethodBool(QStringLiteral("org.xfce.ScreenSaver"),
                        QStringLiteral("/org/xfce/ScreenSaver"),
                        QStringLiteral("org.xfce.ScreenSaver"),
                        QStringLiteral("GetActive"))) {
        m_isLocked = true;
        return;
    }

    // 6. Check FreeDesktop / KDE
    if (queryMethodBool(QStringLiteral("org.freedesktop.ScreenSaver"),
                        QStringLiteral("/org/freedesktop/ScreenSaver"),
                        QStringLiteral("org.freedesktop.ScreenSaver"),
                        QStringLiteral("GetActive"))) {
        m_isLocked = true;
        return;
    }
}

void ScreenLockMonitor::onGnomeActiveChanged(bool active) {
    if (m_isLocked != active) {
        m_isLocked = active;
        emit lockStateChanged(m_isLocked);
    }
}

void ScreenLockMonitor::onFreedesktopActiveChanged(bool active) {
    if (m_isLocked != active) {
        m_isLocked = active;
        emit lockStateChanged(m_isLocked);
    }
}

void ScreenLockMonitor::onLogindSessionLocked() {
    onFreedesktopActiveChanged(true);
}

void ScreenLockMonitor::onLogindSessionUnlocked() {
    onFreedesktopActiveChanged(false);
}
#endif
