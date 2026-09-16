#include "IdleMonitor.h"
#include <QProcessEnvironment>
#include <QDebug>
#include <QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#else
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusConnection>
#endif

IdleMonitor::IdleMonitor(QObject* parent)
    : QObject(parent) {
#ifndef Q_OS_WIN
    const auto env = QProcessEnvironment::systemEnvironment();
    m_desktopEnvironment = env.value(QStringLiteral("XDG_CURRENT_DESKTOP")).toLower();
    if (m_desktopEnvironment.isEmpty()) {
        m_desktopEnvironment = env.value(QStringLiteral("DESKTOP_SESSION")).toLower();
    }
#endif
}

int IdleMonitor::getIdleSeconds() const {
#ifdef Q_OS_WIN
    return getWindowsIdleSeconds();
#else
    // 1. If GNOME / Mutter (Wayland or X11)
    if (m_desktopEnvironment.contains(QStringLiteral("gnome")) ||
        m_desktopEnvironment.contains(QStringLiteral("mutter")) ||
        m_desktopEnvironment.contains(QStringLiteral("unity")) ||
        m_desktopEnvironment.contains(QStringLiteral("pantheon")) ||
        m_desktopEnvironment.contains(QStringLiteral("budgie"))) {
        int sec = getGnomeIdleSeconds();
        if (sec >= 0) return sec;
    }

    // 2. If KDE / Plasma
    if (m_desktopEnvironment.contains(QStringLiteral("kde")) ||
        m_desktopEnvironment.contains(QStringLiteral("plasma"))) {
        int sec = getKdeIdleSeconds();
        if (sec >= 0) return sec;
    }

    // 3. General FreeDesktop / X11
    int secX11 = getX11IdleSeconds();
    if (secX11 >= 0) return secX11;

    // 4. Systemd logind fallback
    int secLogind = getLogindIdleSeconds();
    if (secLogind >= 0) return secLogind;

    // 5. If GNOME Mutter check wasn't tried yet, try it as generic fallback
    int secMutterFallback = getGnomeIdleSeconds();
    if (secMutterFallback >= 0) return secMutterFallback;

    return 0;
#endif
}

bool IdleMonitor::isMediaPlaying() const {
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (now - m_lastMediaCheckTime < 2) {
        return m_cachedMediaPlaying;
    }
    m_lastMediaCheckTime = now;

#ifdef Q_OS_WIN
    m_cachedMediaPlaying = isWindowsMediaPlaying();
#else
    m_cachedMediaPlaying = isLinuxMediaInhibited() || isLinuxAudioPlaying();
#endif

    return m_cachedMediaPlaying;
}

bool IdleMonitor::isUserIdle(int thresholdSec) const {
    if (thresholdSec <= 0) return false;

    // Fast check: get user input idle duration
    const int idleSec = getIdleSeconds();
    if (idleSec < thresholdSec) {
        return false;
    }

    // If user has been inactive for >= thresholdSec, check if media is keeping them engaged
    if (isMediaPlaying()) {
        return false;
    }

    return true;
}

#ifdef Q_OS_WIN
int IdleMonitor::getWindowsIdleSeconds() const {
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&lii)) {
        const DWORD tick = GetTickCount();
        if (tick >= lii.dwTime) {
            return static_cast<int>((tick - lii.dwTime) / 1000);
        }
    }
    return 0;
}

bool IdleMonitor::isWindowsMediaPlaying() const {
    // Check Windows Core Audio for active playback sessions
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool needUninit = SUCCEEDED(hr);

    IMMDeviceEnumerator* pEnum = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&pEnum));
    if (FAILED(hr) || !pEnum) {
        if (needUninit) CoUninitialize();
        return false;
    }

    IMMDevice* pDevice = nullptr;
    hr = pEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
    pEnum->Release();
    if (FAILED(hr) || !pDevice) {
        if (needUninit) CoUninitialize();
        return false;
    }

    IAudioSessionManager2* pMgr = nullptr;
    hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr,
                           reinterpret_cast<void**>(&pMgr));
    pDevice->Release();
    if (FAILED(hr) || !pMgr) {
        if (needUninit) CoUninitialize();
        return false;
    }

    IAudioSessionEnumerator* pSessionEnum = nullptr;
    hr = pMgr->GetSessionEnumerator(&pSessionEnum);
    pMgr->Release();
    if (FAILED(hr) || !pSessionEnum) {
        if (needUninit) CoUninitialize();
        return false;
    }

    int sessionCount = 0;
    pSessionEnum->GetCount(&sessionCount);
    const DWORD currentPid = GetCurrentProcessId();
    bool activeAudio = false;

    for (int i = 0; i < sessionCount; ++i) {
        IAudioSessionControl* pSession = nullptr;
        if (SUCCEEDED(pSessionEnum->GetSession(i, &pSession)) && pSession) {
            IAudioSessionControl2* pSession2 = nullptr;
            if (SUCCEEDED(pSession->QueryInterface(__uuidof(IAudioSessionControl2), reinterpret_cast<void**>(&pSession2))) && pSession2) {
                DWORD pid = 0;
                pSession2->GetProcessId(&pid);
                if (pid != currentPid) {
                    AudioSessionState state;
                    if (SUCCEEDED(pSession2->GetState(&state)) && state == AudioSessionStateActive) {
                        activeAudio = true;
                    }
                }
                pSession2->Release();
            }
            pSession->Release();
        }
        if (activeAudio) break;
    }

    pSessionEnum->Release();
    if (needUninit) CoUninitialize();
    return activeAudio;
}

#else

int IdleMonitor::getGnomeIdleSeconds() const {
    QDBusInterface mutter(
        QStringLiteral("org.gnome.Mutter.IdleMonitor"),
        QStringLiteral("/org/gnome/Mutter/IdleMonitor/Core"),
        QStringLiteral("org.gnome.Mutter.IdleMonitor"),
        QDBusConnection::sessionBus()
    );
    if (mutter.isValid()) {
        QDBusReply<qulonglong> reply = mutter.call(QStringLiteral("GetIdletime"));
        if (reply.isValid()) {
            return static_cast<int>(reply.value() / 1000);
        }
    }
    return -1;
}

int IdleMonitor::getKdeIdleSeconds() const {
    QDBusInterface krunner(
        QStringLiteral("org.freedesktop.ScreenSaver"),
        QStringLiteral("/org/freedesktop/ScreenSaver"),
        QStringLiteral("org.freedesktop.ScreenSaver"),
        QDBusConnection::sessionBus()
    );
    if (krunner.isValid()) {
        QDBusReply<uint> reply = krunner.call(QStringLiteral("GetSessionIdleTime"));
        if (reply.isValid()) {
            return static_cast<int>(reply.value() / 1000);
        }
    }
    return -1;
}

int IdleMonitor::getX11IdleSeconds() const {
    QDBusInterface ss(
        QStringLiteral("org.freedesktop.ScreenSaver"),
        QStringLiteral("/ScreenSaver"),
        QStringLiteral("org.freedesktop.ScreenSaver"),
        QDBusConnection::sessionBus()
    );
    if (ss.isValid()) {
        QDBusReply<uint> reply = ss.call(QStringLiteral("GetSessionIdleTime"));
        if (reply.isValid()) {
            return static_cast<int>(reply.value() / 1000);
        }
    }
    return -1;
}

int IdleMonitor::getLogindIdleSeconds() const {
    QDBusInterface logind(
        QStringLiteral("org.freedesktop.login1"),
        QStringLiteral("/org/freedesktop/login1/session/auto"),
        QStringLiteral("org.freedesktop.login1.Session"),
        QDBusConnection::systemBus()
    );
    if (logind.isValid()) {
        QVariant idleHintVar = logind.property("IdleSinceHintMonotonic");
        if (idleHintVar.isValid()) {
            qulonglong idleMicro = idleHintVar.toULongLong();
            if (idleMicro > 0) {
                // Monotonic microseconds
                struct timespec ts;
                if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
                    qulonglong currentMicro = (static_cast<qulonglong>(ts.tv_sec) * 1000000ULL) + (ts.tv_nsec / 1000);
                    if (currentMicro >= idleMicro) {
                        return static_cast<int>((currentMicro - idleMicro) / 1000000ULL);
                    }
                }
            }
        }
    }
    return -1;
}

bool IdleMonitor::isLinuxMediaInhibited() const {
    // 1. Check GNOME SessionManager IsInhibited with flag 8 (Inhibit suspending/screensaver)
    QDBusInterface sm(
        QStringLiteral("org.gnome.SessionManager"),
        QStringLiteral("/org/gnome/SessionManager"),
        QStringLiteral("org.gnome.SessionManager"),
        QDBusConnection::sessionBus()
    );
    if (sm.isValid()) {
        QDBusReply<bool> reply = sm.call(QStringLiteral("IsInhibited"), static_cast<uint>(8));
        if (reply.isValid() && reply.value()) {
            return true;
        }
    }

    // 2. Check KDE Solid / PowerManagement PolicyAgent
    QDBusInterface kdePolicy(
        QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
        QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
        QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
        QDBusConnection::sessionBus()
    );
    if (kdePolicy.isValid()) {
        QDBusReply<bool> reply = kdePolicy.call(QStringLiteral("hasInhibition"), QStringLiteral("ScreenSaver"));
        if (reply.isValid() && reply.value()) {
            return true;
        }
    }

    return false;
}

bool IdleMonitor::isLinuxAudioPlaying() const {
    // Query pactl for active sink-inputs
    QProcess proc;
    proc.start(QStringLiteral("pactl"), {QStringLiteral("list"), QStringLiteral("sink-inputs")});
    if (!proc.waitForFinished(300)) {
        proc.kill();
        return false;
    }

    const QString output = QString::fromUtf8(proc.readAllStandardOutput());
    const QStringList lines = output.split(QLatin1Char('\n'));

    bool inSink = false;
    bool isCorked = false;
    bool isMuted = false;
    bool isEventRole = false;
    bool isSelfApp = false;

    auto checkSink = [&]() -> bool {
        if (inSink && !isCorked && !isMuted && !isEventRole && !isSelfApp) {
            return true;
        }
        return false;
    };

    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("Sink Input #"))) {
            if (checkSink()) {
                return true;
            }
            inSink = true;
            isCorked = false;
            isMuted = false;
            isEventRole = false;
            isSelfApp = false;
        } else if (inSink) {
            if (trimmed.startsWith(QStringLiteral("Corked:"))) {
                isCorked = trimmed.contains(QStringLiteral("yes"), Qt::CaseInsensitive);
            } else if (trimmed.startsWith(QStringLiteral("Mute:"))) {
                isMuted = trimmed.contains(QStringLiteral("yes"), Qt::CaseInsensitive);
            } else if (trimmed.contains(QStringLiteral("media.role = \"event\"")) ||
                       trimmed.contains(QStringLiteral("media.role = \"notification\""))) {
                isEventRole = true;
            } else if (trimmed.contains(QStringLiteral("application.name = \"ProtectEye\"")) ||
                       trimmed.contains(QStringLiteral("node.name = \"ProtectEye\""))) {
                isSelfApp = true;
            }
        }
    }

    if (checkSink()) {
        return true;
    }

    return false;
}

#endif
