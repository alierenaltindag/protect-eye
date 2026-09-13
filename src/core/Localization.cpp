#include "Localization.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLocale>
#include <QRandomGenerator>
#include <QGuiApplication>
#include <QDebug>
#include <cmath>

static ExerciseVisualType parseVisualType(const QString& str) {
    if (str == QStringLiteral("TrackingInfinity")) return ExerciseVisualType::TrackingInfinity;
    if (str == QStringLiteral("TrackingCircle")) return ExerciseVisualType::TrackingCircle;
    if (str == QStringLiteral("TrackingHorizontal")) return ExerciseVisualType::TrackingHorizontal;
    if (str == QStringLiteral("TrackingVertical")) return ExerciseVisualType::TrackingVertical;
    if (str == QStringLiteral("BreathingCircle")) return ExerciseVisualType::BreathingCircle;
    if (str == QStringLiteral("DistanceFocus")) return ExerciseVisualType::DistanceFocus;
    if (str == QStringLiteral("NearFarShift")) return ExerciseVisualType::NearFarShift;
    if (str == QStringLiteral("SqueezeBlink")) return ExerciseVisualType::SqueezeBlink;
    if (str == QStringLiteral("PeripheralExpansion")) return ExerciseVisualType::PeripheralExpansion;
    return ExerciseVisualType::None;
}

Localization& Localization::instance() {
    static Localization s_instance;
    return s_instance;
}

Localization::Localization(QObject* parent)
    : QObject(parent) {
    ensureLoaded();
}

void Localization::ensureLoaded() {
    if (m_initialized) return;
    m_initialized = true;

    // Load fallback English catalog first
    if (!loadCatalog(QStringLiteral("en"), m_fallbackCatalog)) {
        qWarning() << "Localization: Failed to load fallback English catalog from :/i18n/en.json";
    }

    // Discover supported language files dynamically from resources
    QDir i18nDir(QStringLiteral(":/i18n"));
    QStringList langCodes;
    const auto fileList = i18nDir.entryList({QStringLiteral("*.json")}, QDir::Files);
    for (const auto& f : fileList) {
        if (f.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
            QString code = f.left(f.length() - 5);
            if (!langCodes.contains(code)) {
                langCodes.append(code);
            }
        }
    }

    if (langCodes.isEmpty()) {
        langCodes = { QStringLiteral("en"), QStringLiteral("tr") };
    }

    // Ensure "en" is first in list for consistent display order
    if (langCodes.contains(QStringLiteral("en"))) {
        langCodes.removeAll(QStringLiteral("en"));
        langCodes.prepend(QStringLiteral("en"));
    }

    m_availableLanguages.clear();
    for (const auto& code : langCodes) {
        LanguageCatalog cat;
        if (loadCatalog(code, cat)) {
            m_availableLanguages.append({ cat.code, cat.name });
        }
    }

    switchActiveCatalog();
}

bool Localization::loadCatalog(const QString& code, LanguageCatalog& catalog) {
    const QString path = QStringLiteral(":/i18n/%1.json").arg(code);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Localization: Could not open translation file:" << path;
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Localization: JSON parse error in" << path << ":" << parseError.errorString();
        return false;
    }

    const QJsonObject root = doc.object();
    catalog.code = root.value(QStringLiteral("code")).toString(code);
    catalog.name = root.value(QStringLiteral("name")).toString(code);

    // Strings
    catalog.strings.clear();
    const QJsonObject strObj = root.value(QStringLiteral("strings")).toObject();
    for (auto it = strObj.begin(); it != strObj.end(); ++it) {
        catalog.strings.insert(it.key(), it.value().toString());
    }

    // Short break tips
    catalog.shortTips.clear();
    const QJsonArray shortTipsArr = root.value(QStringLiteral("short_tips")).toArray();
    for (const auto& item : shortTipsArr) {
        catalog.shortTips.append(item.toString());
    }

    // Long break tips
    catalog.longTips.clear();
    const QJsonArray longTipsArr = root.value(QStringLiteral("long_tips")).toArray();
    for (const auto& item : longTipsArr) {
        catalog.longTips.append(item.toString());
    }

    // Short exercises
    catalog.shortExercises.clear();
    const QJsonArray shortExArr = root.value(QStringLiteral("short_exercises")).toArray();
    for (const auto& item : shortExArr) {
        const QJsonObject exObj = item.toObject();
        ExerciseGuide guide;
        guide.visualType = parseVisualType(exObj.value(QStringLiteral("visual_type")).toString());
        guide.badge = exObj.value(QStringLiteral("badge")).toString();
        guide.title = exObj.value(QStringLiteral("title")).toString();
        guide.instruction = exObj.value(QStringLiteral("instruction")).toString();
        catalog.shortExercises.append(guide);
    }

    // Long exercises
    catalog.longExercises.clear();
    const QJsonArray longExArr = root.value(QStringLiteral("long_exercises")).toArray();
    for (const auto& item : longExArr) {
        const QJsonObject exObj = item.toObject();
        ExerciseGuide guide;
        guide.visualType = parseVisualType(exObj.value(QStringLiteral("visual_type")).toString());
        guide.badge = exObj.value(QStringLiteral("badge")).toString();
        guide.title = exObj.value(QStringLiteral("title")).toString();
        guide.instruction = exObj.value(QStringLiteral("instruction")).toString();
        catalog.longExercises.append(guide);
    }

    return true;
}

namespace {

QString appLanguageToCode(AppLanguage lang) {
    switch (lang) {
    case AppLanguage::English: return QStringLiteral("en");
    case AppLanguage::Turkish: return QStringLiteral("tr");
    case AppLanguage::Chinese: return QStringLiteral("zh");
    case AppLanguage::Hindi: return QStringLiteral("hi");
    case AppLanguage::Spanish: return QStringLiteral("es");
    case AppLanguage::French: return QStringLiteral("fr");
    case AppLanguage::Arabic: return QStringLiteral("ar");
    case AppLanguage::Bengali: return QStringLiteral("bn");
    case AppLanguage::Portuguese: return QStringLiteral("pt");
    case AppLanguage::Russian: return QStringLiteral("ru");
    case AppLanguage::Japanese: return QStringLiteral("ja");
    case AppLanguage::Auto:
    default: return QStringLiteral("auto");
    }
}

AppLanguage codeToAppLanguage(const QString& code) {
    if (code == QStringLiteral("en")) return AppLanguage::English;
    if (code == QStringLiteral("tr")) return AppLanguage::Turkish;
    if (code == QStringLiteral("zh")) return AppLanguage::Chinese;
    if (code == QStringLiteral("hi")) return AppLanguage::Hindi;
    if (code == QStringLiteral("es")) return AppLanguage::Spanish;
    if (code == QStringLiteral("fr")) return AppLanguage::French;
    if (code == QStringLiteral("ar")) return AppLanguage::Arabic;
    if (code == QStringLiteral("bn")) return AppLanguage::Bengali;
    if (code == QStringLiteral("pt")) return AppLanguage::Portuguese;
    if (code == QStringLiteral("ru")) return AppLanguage::Russian;
    if (code == QStringLiteral("ja")) return AppLanguage::Japanese;
    return AppLanguage::Auto;
}

} // namespace

void Localization::switchActiveCatalog() {
    const QString effCode = effectiveLanguageCode();
    if (effCode == QStringLiteral("en")) {
        m_activeCatalog = m_fallbackCatalog;
    } else {
        if (!loadCatalog(effCode, m_activeCatalog)) {
            m_activeCatalog = m_fallbackCatalog;
        }
    }

    if (effCode == QStringLiteral("ar")) {
        QGuiApplication::setLayoutDirection(Qt::RightToLeft);
    } else {
        QGuiApplication::setLayoutDirection(Qt::LeftToRight);
    }
}

void Localization::setLanguage(AppLanguage lang) {
    if (m_language != lang) {
        m_language = lang;
        m_languageCode = appLanguageToCode(lang);
        switchActiveCatalog();
        emit languageChanged();
    }
}

void Localization::setLanguageByCode(const QString& code) {
    const QString normalized = code.trimmed().toLower();
    if (m_languageCode != normalized) {
        m_languageCode = normalized;
        m_language = codeToAppLanguage(normalized);
        switchActiveCatalog();
        emit languageChanged();
    }
}

QString Localization::effectiveLanguageCode() const {
    if (m_languageCode.isEmpty() || m_languageCode == QStringLiteral("auto")) {
        const QStringList uiLangs = QLocale::system().uiLanguages();
        for (const QString& tag : uiLangs) {
            QString code = tag.left(2).toLower();
            for (const auto& lang : m_availableLanguages) {
                if (lang.code == code) {
                    return code;
                }
            }
        }
        const QString sysName = QLocale::system().name().toLower();
        for (const auto& lang : m_availableLanguages) {
            if (sysName.startsWith(lang.code)) {
                return lang.code;
            }
        }
        return QStringLiteral("en");
    }
    return m_languageCode;
}

AppLanguage Localization::effectiveLanguage() const {
    return codeToAppLanguage(effectiveLanguageCode());
}

bool Localization::isTurkish() const {
    return effectiveLanguageCode() == QStringLiteral("tr");
}

QList<LanguageInfo> Localization::availableLanguages() const {
    const_cast<Localization*>(this)->ensureLoaded();
    return m_availableLanguages;
}

QString Localization::get(const QString& key, const QString& fallback) const {
    const_cast<Localization*>(this)->ensureLoaded();

    if (m_activeCatalog.strings.contains(key)) {
        const QString& val = m_activeCatalog.strings.value(key);
        if (!val.isEmpty()) return val;
    }
    if (m_fallbackCatalog.strings.contains(key)) {
        return m_fallbackCatalog.strings.value(key);
    }
    return fallback;
}

// ----------------------------------------------------------------------------
// Strings
// ----------------------------------------------------------------------------

QString Localization::appName() const {
    return get(QStringLiteral("app_name"), QStringLiteral("ProtectEye"));
}

QString Localization::trayTooltip() const {
    return get(QStringLiteral("tray_tooltip"));
}

QString Localization::statusActive() const {
    return get(QStringLiteral("status_active"));
}

QString Localization::statusPaused() const {
    return get(QStringLiteral("status_paused"));
}

QString Localization::statusInBreak() const {
    return get(QStringLiteral("status_in_break"));
}

QString Localization::statusSnoozed() const {
    return get(QStringLiteral("status_snoozed"));
}

QString Localization::shortBreakLabel(const QString& time) const {
    return get(QStringLiteral("short_break_label")).arg(time);
}

QString Localization::longBreakLabel(const QString& time) const {
    return get(QStringLiteral("long_break_label")).arg(time);
}

QString Localization::actionPause() const {
    return get(QStringLiteral("action_pause"));
}

QString Localization::actionResume() const {
    return get(QStringLiteral("action_resume"));
}

QString Localization::actionTakeShortNow() const {
    return get(QStringLiteral("action_take_short_now"));
}

QString Localization::actionTakeLongNow() const {
    return get(QStringLiteral("action_take_long_now"));
}

QString Localization::actionSettings() const {
    return get(QStringLiteral("action_settings"));
}

QString Localization::actionQuit() const {
    return get(QStringLiteral("action_quit"));
}

// ----------------------------------------------------------------------------
// Notifications
// ----------------------------------------------------------------------------

QString Localization::startupNotificationTitle() const {
    return get(QStringLiteral("startup_notification_title"), QStringLiteral("ProtectEye"));
}

QString Localization::startupNotificationMessage() const {
    return get(QStringLiteral("startup_notification_message"));
}

QString Localization::alreadyRunningTitle() const {
    return get(QStringLiteral("already_running_title"), QStringLiteral("ProtectEye"));
}

QString Localization::alreadyRunningMessage() const {
    return get(QStringLiteral("already_running_message"));
}

QString Localization::preBreakTitle(bool isLong) const {
    return isLong
        ? get(QStringLiteral("pre_break_title_long"))
        : get(QStringLiteral("pre_break_title_short"));
}

QString Localization::preBreakMessage(int secondsLeft) const {
    return get(QStringLiteral("pre_break_message")).arg(secondsLeft);
}

// ----------------------------------------------------------------------------
// Overlay
// ----------------------------------------------------------------------------

QString Localization::badgeShortBreak() const {
    return get(QStringLiteral("badge_short_break"));
}

QString Localization::badgeLongBreak() const {
    return get(QStringLiteral("badge_long_break"));
}

QString Localization::buttonSnooze(int minutes) const {
    return get(QStringLiteral("button_snooze")).arg(minutes);
}

QString Localization::buttonSkip() const {
    return get(QStringLiteral("button_skip"));
}

// ----------------------------------------------------------------------------
// Settings Dialog
// ----------------------------------------------------------------------------

QString Localization::settingsTitle() const {
    return get(QStringLiteral("settings_title"));
}

QString Localization::settingsHeader() const {
    return get(QStringLiteral("settings_header"));
}

QString Localization::groupBreakSchedules() const {
    return get(QStringLiteral("group_break_schedules"));
}

QString Localization::groupSmartFeatures() const {
    return get(QStringLiteral("group_smart_features"));
}

QString Localization::groupLanguage() const {
    return get(QStringLiteral("group_language"));
}

QString Localization::labelLanguage() const {
    return get(QStringLiteral("label_language"));
}

QString Localization::langAuto() const {
    return get(QStringLiteral("lang_auto"));
}

QString Localization::langEnglish() const {
    return get(QStringLiteral("lang_english"), QStringLiteral("English"));
}

QString Localization::langTurkish() const {
    return get(QStringLiteral("lang_turkish"), QStringLiteral("Türkçe"));
}

QString Localization::labelShortInterval() const {
    return get(QStringLiteral("label_short_interval"));
}

QString Localization::labelShortDuration() const {
    return get(QStringLiteral("label_short_duration"));
}

QString Localization::labelLongInterval() const {
    return get(QStringLiteral("label_long_interval"));
}

QString Localization::labelLongDuration() const {
    return get(QStringLiteral("label_long_duration"));
}

QString Localization::labelSnoozeDuration() const {
    return get(QStringLiteral("label_snooze_duration"));
}

QString Localization::labelPreWarn() const {
    return get(QStringLiteral("label_pre_warn"));
}

QString Localization::unitMinutes() const {
    return get(QStringLiteral("unit_minutes"));
}

QString Localization::unitSeconds() const {
    return get(QStringLiteral("unit_seconds"));
}

QString Localization::checkSound() const {
    return get(QStringLiteral("check_sound"));
}

QString Localization::checkDnd() const {
    return get(QStringLiteral("check_dnd"));
}

QString Localization::checkLock() const {
    return get(QStringLiteral("check_lock"));
}

QString Localization::checkAutostart() const {
    return get(QStringLiteral("check_autostart"));
}

QString Localization::checkInteractiveExercises() const {
    return get(QStringLiteral("check_interactive_exercises"));
}

QString Localization::buttonDefaults() const {
    return get(QStringLiteral("button_defaults"));
}

QString Localization::buttonCancel() const {
    return get(QStringLiteral("button_cancel"));
}

QString Localization::buttonSave() const {
    return get(QStringLiteral("button_save"));
}

// ----------------------------------------------------------------------------
// Guidance & Phases
// ----------------------------------------------------------------------------

QString Localization::breathInhale() const {
    return get(QStringLiteral("breath_inhale"));
}

QString Localization::breathHold() const {
    return get(QStringLiteral("breath_hold"));
}

QString Localization::breathExhale() const {
    return get(QStringLiteral("breath_exhale"));
}

QString Localization::squeezePhaseClose() const {
    return get(QStringLiteral("squeeze_phase_close"));
}

QString Localization::squeezePhaseSqueeze() const {
    return get(QStringLiteral("squeeze_phase_squeeze"));
}

QString Localization::squeezePhaseOpen() const {
    return get(QStringLiteral("squeeze_phase_open"));
}

QString Localization::focusNearLabel() const {
    return get(QStringLiteral("focus_near_label"));
}

QString Localization::focusFarLabel() const {
    return get(QStringLiteral("focus_far_label"));
}

// ----------------------------------------------------------------------------
// Exercises & Tips
// ----------------------------------------------------------------------------

QList<ExerciseGuide> Localization::allShortBreakExercises() const {
    const_cast<Localization*>(this)->ensureLoaded();
    if (!m_activeCatalog.shortExercises.isEmpty()) {
        return m_activeCatalog.shortExercises;
    }
    return m_fallbackCatalog.shortExercises;
}

QList<ExerciseGuide> Localization::allLongBreakExercises() const {
    const_cast<Localization*>(this)->ensureLoaded();
    if (!m_activeCatalog.longExercises.isEmpty()) {
        return m_activeCatalog.longExercises;
    }
    return m_fallbackCatalog.longExercises;
}

ExerciseGuide Localization::getShortBreakExercise(int cycleIndex) const {
    const auto list = allShortBreakExercises();
    if (list.isEmpty()) return ExerciseGuide();
    int idx = std::abs(cycleIndex) % list.size();
    return list.at(idx);
}

ExerciseGuide Localization::getLongBreakExercise(int cycleIndex) const {
    const auto list = allLongBreakExercises();
    if (list.isEmpty()) return ExerciseGuide();
    int idx = std::abs(cycleIndex) % list.size();
    return list.at(idx);
}

QStringList Localization::shortBreakTips() const {
    const_cast<Localization*>(this)->ensureLoaded();
    if (!m_activeCatalog.shortTips.isEmpty()) {
        return m_activeCatalog.shortTips;
    }
    return m_fallbackCatalog.shortTips;
}

QStringList Localization::longBreakTips() const {
    const_cast<Localization*>(this)->ensureLoaded();
    if (!m_activeCatalog.longTips.isEmpty()) {
        return m_activeCatalog.longTips;
    }
    return m_fallbackCatalog.longTips;
}

QString Localization::getRandomTip(bool isLong) const {
    const QStringList& list = isLong ? longBreakTips() : shortBreakTips();
    if (list.isEmpty()) {
        return get(QStringLiteral("default_tip"), QStringLiteral("Rest your eyes and take a calm breath."));
    }
    int idx = QRandomGenerator::global()->bounded(list.size());
    return list.at(idx);
}
