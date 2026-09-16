#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QList>

enum class AppLanguage {
    Auto,
    English,
    Turkish,
    Chinese,
    Hindi,
    Spanish,
    French,
    Arabic,
    Bengali,
    Portuguese,
    Russian,
    Japanese
};

enum class ExerciseVisualType {
    None,
    TrackingInfinity,
    TrackingCircle,
    TrackingHorizontal,
    TrackingVertical,
    BreathingCircle,
    DistanceFocus,
    NearFarShift,
    SqueezeBlink,
    PeripheralExpansion
};

struct ExerciseGuide {
    ExerciseVisualType visualType{ExerciseVisualType::None};
    QString badge;
    QString title;
    QString instruction;
};

struct LanguageInfo {
    QString code; // e.g. "en", "tr"
    QString name; // e.g. "English", "Türkçe"
};

class Localization : public QObject {
    Q_OBJECT

public:
    static Localization& instance();

    void setLanguage(AppLanguage lang);
    void setLanguageByCode(const QString& code);
    AppLanguage language() const { return m_language; }
    QString languageCode() const { return m_languageCode; }
    AppLanguage effectiveLanguage() const;
    QString effectiveLanguageCode() const;
    bool isTurkish() const;

    // Available languages registered in the system
    QList<LanguageInfo> availableLanguages() const;

    // Generic key-based translation lookup
    QString get(const QString& key, const QString& fallback = QString()) const;

    // Translated strings
    QString appName() const;
    QString trayTooltip() const;
    QString statusActive() const;
    QString statusPaused() const;
    QString statusInBreak() const;
    QString statusSnoozed() const;
    QString shortBreakLabel(const QString& time) const;
    QString longBreakLabel(const QString& time) const;
    QString actionPause() const;
    QString actionResume() const;
    QString actionTakeShortNow() const;
    QString actionTakeLongNow() const;
    QString actionSettings() const;
    QString actionQuit() const;

    // Startup notification
    QString startupNotificationTitle() const;
    QString startupNotificationMessage() const;

    // Already running notification
    QString alreadyRunningTitle() const;
    QString alreadyRunningMessage() const;

    // Pre-break notification
    QString preBreakTitle(bool isLong) const;
    QString preBreakMessage(int secondsLeft) const;

    // Overlay
    QString badgeShortBreak() const;
    QString badgeLongBreak() const;
    QString buttonSnooze(int minutes) const;
    QString buttonSkip() const;

    // Settings dialog
    QString settingsTitle() const;
    QString settingsHeader() const;
    QString tabSchedules() const;
    QString tabSmartFeatures() const;
    QString tabGeneral() const;
    QString groupBreakSchedules() const;
    QString groupSmartFeatures() const;
    QString groupLanguage() const;
    QString groupUpdates() const;
    QString labelLanguage() const;
    QString langAuto() const;
    QString langEnglish() const;
    QString langTurkish() const;
    QString labelShortInterval() const;
    QString labelShortDuration() const;
    QString labelLongInterval() const;
    QString labelLongDuration() const;
    QString labelSnoozeDuration() const;
    QString labelPreWarn() const;
    QString unitMinutes() const;
    QString unitSeconds() const;
    QString checkSound() const;
    QString checkDnd() const;
    QString checkLock() const;
    QString checkAutostart() const;
    QString checkIdle() const;
    QString checkInteractiveExercises() const;
    QString checkUpdates() const;
    QString btnCheckUpdates() const;
    QString updateStatusChecking() const;
    QString updateStatusUpToDate() const;
    QString updateStatusAvailable() const;
    QString updateStatusFailed() const;
    QString actionUpdateAvailable() const;
    QString updateNotifTitle() const;
    QString updateNotifBody() const;
    QString buttonDefaults() const;
    QString buttonCancel() const;
    QString buttonSave() const;

    // Breathing guidance labels
    QString breathInhale() const;
    QString breathHold() const;
    QString breathExhale() const;

    // Squeeze Blink & Focus Shift guidance labels
    QString squeezePhaseClose() const;
    QString squeezePhaseSqueeze() const;
    QString squeezePhaseOpen() const;
    QString focusNearLabel() const;
    QString focusFarLabel() const;

    // Interactive & Dynamic Exercise Guides
    ExerciseGuide getShortBreakExercise(int cycleIndex) const;
    ExerciseGuide getLongBreakExercise(int cycleIndex) const;
    QList<ExerciseGuide> allShortBreakExercises() const;
    QList<ExerciseGuide> allLongBreakExercises() const;

    // Break Tips
    QString getRandomTip(bool isLong) const;
    QStringList shortBreakTips() const;
    QStringList longBreakTips() const;

signals:
    void languageChanged();

private:
    explicit Localization(QObject* parent = nullptr);
    ~Localization() override = default;
    Localization(const Localization&) = delete;
    Localization& operator=(const Localization&) = delete;

    struct LanguageCatalog {
        QString code;
        QString name;
        QHash<QString, QString> strings;
        QList<ExerciseGuide> shortExercises;
        QList<ExerciseGuide> longExercises;
        QStringList shortTips;
        QStringList longTips;
    };

    void ensureLoaded();
    bool loadCatalog(const QString& code, LanguageCatalog& catalog);
    void switchActiveCatalog();

    AppLanguage m_language{AppLanguage::Auto};
    QString m_languageCode{QStringLiteral("auto")};

    LanguageCatalog m_fallbackCatalog; // English
    LanguageCatalog m_activeCatalog;
    QList<LanguageInfo> m_availableLanguages;
    bool m_initialized{false};
};
