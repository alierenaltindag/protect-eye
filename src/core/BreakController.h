#pragma once

#include <QObject>
#include <QTimer>
#include <memory>
class DndMonitor;
class ScreenLockMonitor;
class IdleMonitor;

enum class BreakState {
    Running,
    Paused,
    InShortBreak,
    InLongBreak,
    Snoozed
};

class BreakController : public QObject {
    Q_OBJECT

public:
    explicit BreakController(DndMonitor* dndMonitor, ScreenLockMonitor* lockMonitor, IdleMonitor* idleMonitor = nullptr, QObject* parent = nullptr);
    ~BreakController() override = default;

    BreakState state() const { return m_state; }
    bool isPaused() const { return m_state == BreakState::Paused; }
    bool isInBreak() const { return m_state == BreakState::InShortBreak || m_state == BreakState::InLongBreak; }
    bool isUserIdle() const { return m_isUserIdle; }

    int secondsUntilShortBreak() const { return m_secToShortBreak; }
    int secondsUntilLongBreak() const { return m_secToLongBreak; }
    int breakRemainingSeconds() const { return m_breakRemainingSec; }
    int breakTotalDuration() const { return m_breakTotalSec; }
    bool isCurrentBreakLong() const { return m_state == BreakState::InLongBreak; }

public slots:
    void start();
    void pause();
    void resume();
    void togglePause();

    void triggerBreakNow(bool isLong = false);
    void skipBreak();
    void snoozeBreak(int seconds = 120);

    void onSettingsChanged();

signals:
    void stateChanged(BreakState newState);
    void tick(int secToShort, int secToLong);
    void preBreakWarning(bool isLong, int secondsLeft);
    void breakStarted(bool isLong, int durationSec);
    void breakTick(int remainingSec, int totalSec);
    void breakCompleted();
    void breakEnded();
    void breakSilentlySkippedDnd();
    void userIdleStateChanged(bool isIdle);

private slots:
    void onSecondTimer();
    void onLockStateChanged(bool isLocked);

private:
    void startBreak(bool isLong, int durationSec, bool force = false);
    void finishBreak();
    void resetTimers();

    DndMonitor* m_dndMonitor;
    ScreenLockMonitor* m_lockMonitor;
    IdleMonitor* m_idleMonitor{nullptr};
    QTimer m_ticker;

    BreakState m_state{BreakState::Paused};
    BreakState m_preLockState{BreakState::Running};
    bool m_isUserIdle{false};
    qint64 m_idleStartTime{0};

    int m_secToShortBreak{20 * 60};
    int m_secToLongBreak{50 * 60};

    int m_breakRemainingSec{0};
    int m_breakTotalSec{0};
    bool m_preWarningSentShort{false};
    bool m_preWarningSentLong{false};
};

