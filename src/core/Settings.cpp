#include "Settings.h"
#include "Localization.h"
#include "utils/AutostartHelper.h"
#include <QCoreApplication>
#include <algorithm>

Settings& Settings::instance() {
    static Settings s_instance;
    return s_instance;
}

Settings::Settings(QObject* parent)
    : QObject(parent)
    , m_qsettings("ProtectEye", "ProtectEye") {
    load();
}

void Settings::load() {
    m_shortBreakIntervalSec = std::max(1, m_qsettings.value("breaks/short_interval", 20 * 60).toInt());
    m_shortBreakDurationSec = std::max(1, m_qsettings.value("breaks/short_duration", 20).toInt());
    m_longBreakIntervalSec = std::max(1, m_qsettings.value("breaks/long_interval", 50 * 60).toInt());
    m_longBreakDurationSec = std::max(1, m_qsettings.value("breaks/long_duration", 120).toInt());
    m_snoozeDurationSec = std::max(1, m_qsettings.value("breaks/snooze_duration", 120).toInt());
    m_preBreakNotificationSec = std::max(0, m_qsettings.value("breaks/pre_notification", 30).toInt());

    m_soundEnabled = m_qsettings.value("features/sound_enabled", true).toBool();
    m_dndCheckEnabled = m_qsettings.value("features/dnd_check_enabled", true).toBool();
    m_screenLockCheckEnabled = m_qsettings.value("features/screen_lock_check_enabled", true).toBool();

    bool hasAutostartKey = m_qsettings.contains("features/autostart_enabled");
    m_autostartEnabled = m_qsettings.value("features/autostart_enabled", true).toBool();
    m_interactiveExercisesEnabled = m_qsettings.value("features/interactive_exercises_enabled", true).toBool();

    m_language = m_qsettings.value("general/language", QStringLiteral("auto")).toString();
    Localization::instance().setLanguageByCode(m_language);

    if (!hasAutostartKey) {
        // Initial setup on first run: configure autostart on system boot
        m_qsettings.setValue("features/autostart_enabled", true);
        AutostartHelper::setAutostartEnabled(true);
    }
}

void Settings::save() {
    m_qsettings.setValue("breaks/short_interval", m_shortBreakIntervalSec);
    m_qsettings.setValue("breaks/short_duration", m_shortBreakDurationSec);
    m_qsettings.setValue("breaks/long_interval", m_longBreakIntervalSec);
    m_qsettings.setValue("breaks/long_duration", m_longBreakDurationSec);
    m_qsettings.setValue("breaks/snooze_duration", m_snoozeDurationSec);
    m_qsettings.setValue("breaks/pre_notification", m_preBreakNotificationSec);

    m_qsettings.setValue("features/sound_enabled", m_soundEnabled);
    m_qsettings.setValue("features/dnd_check_enabled", m_dndCheckEnabled);
    m_qsettings.setValue("features/screen_lock_check_enabled", m_screenLockCheckEnabled);
    m_qsettings.setValue("features/autostart_enabled", m_autostartEnabled);
    m_qsettings.setValue("features/interactive_exercises_enabled", m_interactiveExercisesEnabled);

    m_qsettings.setValue("general/language", m_language);
    m_qsettings.sync();

    Localization::instance().setLanguageByCode(m_language);

    emit settingsChanged();
}

void Settings::resetToDefaults() {
    m_shortBreakIntervalSec = 20 * 60;
    m_shortBreakDurationSec = 20;
    m_longBreakIntervalSec = 50 * 60;
    m_longBreakDurationSec = 120;
    m_snoozeDurationSec = 120;
    m_preBreakNotificationSec = 30;

    m_soundEnabled = true;
    m_dndCheckEnabled = true;
    m_screenLockCheckEnabled = true;
    m_autostartEnabled = true;
    m_interactiveExercisesEnabled = true;

    m_language = QStringLiteral("auto");
    Localization::instance().setLanguageByCode(m_language);

    save();
}


void Settings::setShortBreakIntervalSec(int sec) {
    int valid = std::max(1, sec);
    if (m_shortBreakIntervalSec != valid) {
        m_shortBreakIntervalSec = valid;
    }
}

void Settings::setShortBreakDurationSec(int sec) {
    int valid = std::max(1, sec);
    if (m_shortBreakDurationSec != valid) {
        m_shortBreakDurationSec = valid;
    }
}

void Settings::setLongBreakIntervalSec(int sec) {
    int valid = std::max(1, sec);
    if (m_longBreakIntervalSec != valid) {
        m_longBreakIntervalSec = valid;
    }
}

void Settings::setLongBreakDurationSec(int sec) {
    int valid = std::max(1, sec);
    if (m_longBreakDurationSec != valid) {
        m_longBreakDurationSec = valid;
    }
}

void Settings::setSnoozeDurationSec(int sec) {
    int valid = std::max(1, sec);
    if (m_snoozeDurationSec != valid) {
        m_snoozeDurationSec = valid;
    }
}

void Settings::setPreBreakNotificationSec(int sec) {
    int valid = std::max(0, sec);
    if (m_preBreakNotificationSec != valid) {
        m_preBreakNotificationSec = valid;
    }
}

void Settings::setSoundEnabled(bool enabled) {
    if (m_soundEnabled != enabled) {
        m_soundEnabled = enabled;
    }
}

void Settings::setDndCheckEnabled(bool enabled) {
    if (m_dndCheckEnabled != enabled) {
        m_dndCheckEnabled = enabled;
    }
}

void Settings::setScreenLockCheckEnabled(bool enabled) {
    if (m_screenLockCheckEnabled != enabled) {
        m_screenLockCheckEnabled = enabled;
    }
}

void Settings::setAutostartEnabled(bool enabled) {
    if (m_autostartEnabled != enabled) {
        m_autostartEnabled = enabled;
    }
}

void Settings::setInteractiveExercisesEnabled(bool enabled) {
    if (m_interactiveExercisesEnabled != enabled) {
        m_interactiveExercisesEnabled = enabled;
    }
}

void Settings::setLanguage(const QString& lang) {
    if (m_language != lang) {
        m_language = lang;
        Localization::instance().setLanguageByCode(m_language);
    }
}

