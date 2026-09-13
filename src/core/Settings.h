#pragma once

#include <QObject>
#include <QSettings>

class Settings : public QObject {
    Q_OBJECT

public:
    static Settings& instance();

    // Intervals & Durations (in seconds)
    int shortBreakIntervalSec() const { return m_shortBreakIntervalSec; }
    void setShortBreakIntervalSec(int sec);

    int shortBreakDurationSec() const { return m_shortBreakDurationSec; }
    void setShortBreakDurationSec(int sec);

    int longBreakIntervalSec() const { return m_longBreakIntervalSec; }
    void setLongBreakIntervalSec(int sec);

    int longBreakDurationSec() const { return m_longBreakDurationSec; }
    void setLongBreakDurationSec(int sec);

    int snoozeDurationSec() const { return m_snoozeDurationSec; }
    void setSnoozeDurationSec(int sec);

    int preBreakNotificationSec() const { return m_preBreakNotificationSec; }
    void setPreBreakNotificationSec(int sec);

    // Feature Toggles
    bool soundEnabled() const { return m_soundEnabled; }
    void setSoundEnabled(bool enabled);

    bool dndCheckEnabled() const { return m_dndCheckEnabled; }
    void setDndCheckEnabled(bool enabled);

    bool screenLockCheckEnabled() const { return m_screenLockCheckEnabled; }
    void setScreenLockCheckEnabled(bool enabled);

    bool autostartEnabled() const { return m_autostartEnabled; }
    void setAutostartEnabled(bool enabled);

    bool interactiveExercisesEnabled() const { return m_interactiveExercisesEnabled; }
    void setInteractiveExercisesEnabled(bool enabled);

    // Language ("auto", "en", "tr")
    QString language() const { return m_language; }
    void setLanguage(const QString& lang);

    void load();
    void save();

    void resetToDefaults();

signals:
    void settingsChanged();

private:
    explicit Settings(QObject* parent = nullptr);
    ~Settings() override = default;
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    QSettings m_qsettings;

    int m_shortBreakIntervalSec{20 * 60}; // 20 min (20-20-20 rule)
    int m_shortBreakDurationSec{20};     // 20 sec
    int m_longBreakIntervalSec{50 * 60};  // 50 min
    int m_longBreakDurationSec{120};     // 120 sec (2 min)
    int m_snoozeDurationSec{120};        // 2 min
    int m_preBreakNotificationSec{30};   // 30 sec

    bool m_soundEnabled{true};
    bool m_dndCheckEnabled{true};
    bool m_screenLockCheckEnabled{true};
    bool m_autostartEnabled{false};
    bool m_interactiveExercisesEnabled{true};

    QString m_language{QStringLiteral("auto")};
};

