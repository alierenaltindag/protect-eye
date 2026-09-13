#include "BreakController.h"
#include "Settings.h"
#include <QDebug>
#include <algorithm>

BreakController::BreakController(DndMonitor* dndMonitor, ScreenLockMonitor* lockMonitor, QObject* parent)
    : QObject(parent)
    , m_dndMonitor(dndMonitor)
    , m_lockMonitor(lockMonitor) {
    connect(&m_ticker, &QTimer::timeout, this, &BreakController::onSecondTimer);
    m_ticker.setInterval(1000);
    m_ticker.setTimerType(Qt::PreciseTimer);

    if (m_lockMonitor) {
        connect(m_lockMonitor, &ScreenLockMonitor::lockStateChanged, this, &BreakController::onLockStateChanged);
    }

    connect(&Settings::instance(), &Settings::settingsChanged, this, &BreakController::onSettingsChanged);
    resetTimers();
}

void BreakController::start() {
    if (m_state == BreakState::Paused) {
        m_state = BreakState::Running;
        m_ticker.start();
        emit stateChanged(m_state);
    }
}

void BreakController::pause() {
    if (isInBreak()) {
        return;
    }
    if (m_state != BreakState::Paused) {
        m_preLockState = m_state;
        m_state = BreakState::Paused;
        m_ticker.stop();
        emit stateChanged(m_state);
    }
}

void BreakController::resume() {
    if (m_state == BreakState::Paused) {
        m_state = (m_preLockState == BreakState::Snoozed) ? BreakState::Snoozed : BreakState::Running;
        m_ticker.start();
        emit stateChanged(m_state);
    }
}

void BreakController::togglePause() {
    if (m_state == BreakState::Paused) {
        resume();
    } else {
        pause();
    }
}

void BreakController::onSettingsChanged() {
    auto& s = Settings::instance();
    m_secToShortBreak = std::min(m_secToShortBreak, s.shortBreakIntervalSec());
    m_secToLongBreak = std::min(m_secToLongBreak, s.longBreakIntervalSec());
}

void BreakController::resetTimers() {
    auto& s = Settings::instance();
    m_secToShortBreak = s.shortBreakIntervalSec();
    m_secToLongBreak = s.longBreakIntervalSec();
    m_preWarningSentShort = false;
    m_preWarningSentLong = false;
}

void BreakController::triggerBreakNow(bool isLong) {
    auto& s = Settings::instance();
    int duration = isLong ? s.longBreakDurationSec() : s.shortBreakDurationSec();
    startBreak(isLong, duration, true);
}

void BreakController::skipBreak() {
    if (isInBreak()) {
        bool wasLong = (m_state == BreakState::InLongBreak);
        m_state = BreakState::Running;
        m_breakRemainingSec = 0;

        if (wasLong) {
            resetTimers();
        } else {
            m_secToShortBreak = Settings::instance().shortBreakIntervalSec();
            m_preWarningSentShort = false;
        }

        emit breakEnded();
        emit stateChanged(m_state);
    }
}

void BreakController::snoozeBreak(int seconds) {
    if (isInBreak()) {
        bool wasLong = (m_state == BreakState::InLongBreak);
        m_state = BreakState::Snoozed;
        m_breakRemainingSec = 0;

        if (wasLong) {
            m_secToLongBreak = seconds;
            m_secToShortBreak = std::max(m_secToShortBreak, seconds + Settings::instance().shortBreakIntervalSec());
            m_preWarningSentLong = false;
        } else {
            m_secToShortBreak = seconds;
            m_secToLongBreak = std::max(m_secToLongBreak, seconds + 60);
            m_preWarningSentShort = false;
        }

        m_ticker.start();
        emit breakEnded();
        emit stateChanged(m_state);
    }
}

void BreakController::startBreak(bool isLong, int durationSec, bool force) {
    // Check DND if enabled and break is not forced
    if (!force && Settings::instance().dndCheckEnabled() && m_dndMonitor && m_dndMonitor->isDndActive()) {
        qDebug() << "DND is active; silently skipping break.";
        emit breakSilentlySkippedDnd();
        if (isLong) {
            resetTimers();
        } else {
            m_secToShortBreak = Settings::instance().shortBreakIntervalSec();
            m_preWarningSentShort = false;
        }
        return;
    }

    m_state = isLong ? BreakState::InLongBreak : BreakState::InShortBreak;
    m_breakTotalSec = durationSec;
    m_breakRemainingSec = durationSec;
    m_ticker.start();

    emit breakStarted(isLong, durationSec);
    emit breakTick(m_breakRemainingSec, m_breakTotalSec);
    emit stateChanged(m_state);
}

void BreakController::finishBreak() {
    bool wasLong = (m_state == BreakState::InLongBreak);
    m_state = BreakState::Running;
    m_breakRemainingSec = 0;

    if (wasLong) {
        resetTimers();
    } else {
        m_secToShortBreak = Settings::instance().shortBreakIntervalSec();
        m_preWarningSentShort = false;
    }

    emit breakCompleted();
    emit breakEnded();
    emit stateChanged(m_state);
}

void BreakController::onSecondTimer() {
    if (m_state == BreakState::Paused) {
        return;
    }

    if (isInBreak()) {
        m_breakRemainingSec--;
        emit breakTick(m_breakRemainingSec, m_breakTotalSec);

        if (m_breakRemainingSec <= 0) {
            finishBreak();
        }
        return;
    }

    // Normal counting or snoozed
    m_secToShortBreak--;
    m_secToLongBreak--;

    emit tick(m_secToShortBreak, m_secToLongBreak);

    // Pre-break warning check (sends notification exactly at 30 seconds before break)
    int warnSec = Settings::instance().preBreakNotificationSec();
    if (warnSec > 0) {
        if (!m_preWarningSentLong && m_secToLongBreak <= warnSec && m_secToLongBreak > 0) {
            emit preBreakWarning(true, m_secToLongBreak);
            m_preWarningSentLong = true;
        } else if (!m_preWarningSentShort && m_secToShortBreak <= warnSec && m_secToShortBreak > 0) {
            emit preBreakWarning(false, m_secToShortBreak);
            m_preWarningSentShort = true;
        }
    }

    // Check long break trigger first
    if (m_secToLongBreak <= 0) {
        auto& s = Settings::instance();
        startBreak(true, s.longBreakDurationSec());
        return;
    }

    // Check short break trigger
    if (m_secToShortBreak <= 0) {
        auto& s = Settings::instance();
        startBreak(false, s.shortBreakDurationSec());
        return;
    }
}

void BreakController::onLockStateChanged(bool isLocked) {
    if (!Settings::instance().screenLockCheckEnabled()) {
        return;
    }

    if (isLocked) {
        if (isInBreak()) {
            skipBreak();
        }
        if (m_state != BreakState::Paused) {
            m_preLockState = m_state;
            pause();
        }
    } else {
        // Unlocked: reset break timers so user isn't immediately interrupted
        m_secToShortBreak = Settings::instance().shortBreakIntervalSec();
        m_secToLongBreak = Settings::instance().longBreakIntervalSec();
        m_preWarningSentShort = false;
        m_preWarningSentLong = false;
        if (m_preLockState == BreakState::Running || m_preLockState == BreakState::Snoozed) {
            resume();
        }
    }
}

