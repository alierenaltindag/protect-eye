#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>

class IdleMonitor : public QObject {
    Q_OBJECT

public:
    explicit IdleMonitor(QObject* parent = nullptr);
    ~IdleMonitor() override = default;

    // Returns seconds since the last user input (keyboard or mouse)
    virtual int getIdleSeconds() const;

    // Returns true if audio or video media is actively playing / inhibiting screensaver
    virtual bool isMediaPlaying() const;

    // Returns true if user has been inactive for at least thresholdSec and no media is playing
    virtual bool isUserIdle(int thresholdSec) const;

private:
#ifdef Q_OS_WIN
    int getWindowsIdleSeconds() const;
    bool isWindowsMediaPlaying() const;
#else
    int getGnomeIdleSeconds() const;
    int getKdeIdleSeconds() const;
    int getX11IdleSeconds() const;
    int getLogindIdleSeconds() const;

    bool isLinuxAudioPlaying() const;
    bool isLinuxMediaInhibited() const;

    QString m_desktopEnvironment;
#endif

    mutable qint64 m_lastMediaCheckTime{0};
    mutable bool m_cachedMediaPlaying{false};
};
