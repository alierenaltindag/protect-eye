#include <QTest>
#include <QSignalSpy>
#include "core/Settings.h"
#include "core/Localization.h"
#include "core/BreakController.h"

#include "services/DndMonitor.h"
#include "services/ScreenLockMonitor.h"
#include "services/UpdateChecker.h"
#include "utils/AutostartHelper.h"
#include "ui/OverlayWindow.h"
#include "ui/OverlayManager.h"
#include "ui/SettingsDialog.h"
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>

// Mock DndMonitor for testing DND logic
class MockDndMonitor : public DndMonitor {
public:
    bool mockDndActive{false};
    bool isDndActive() const override { return mockDndActive; }
};

class MockScreenLockMonitor : public ScreenLockMonitor {
public:
    void triggerLock(bool locked) {
        emit lockStateChanged(locked);
    }
};

class TestProtectEye : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        Q_INIT_RESOURCE(resources);
        QCoreApplication::setOrganizationName("ProtectEyeTest");
        QCoreApplication::setApplicationName("ProtectEyeTest");
    }

    void testSettingsDefaults() {
        auto& s = Settings::instance();
        s.resetToDefaults();

        QCOMPARE(s.shortBreakIntervalSec(), 20 * 60);
        QCOMPARE(s.shortBreakDurationSec(), 20);
        QCOMPARE(s.longBreakIntervalSec(), 50 * 60);
        QCOMPARE(s.longBreakDurationSec(), 120);
        QCOMPARE(s.snoozeDurationSec(), 120);
        QCOMPARE(s.preBreakNotificationSec(), 30);
        QCOMPARE(s.soundEnabled(), true);
        QCOMPARE(s.dndCheckEnabled(), true);
        QCOMPARE(s.screenLockCheckEnabled(), true);
        QCOMPARE(s.autostartEnabled(), true);
    }

    void testSettingsMutation() {
        auto& s = Settings::instance();
        s.setShortBreakIntervalSec(600);
        s.setShortBreakDurationSec(20);
        s.setAutostartEnabled(false);
        s.save();

        QCOMPARE(s.shortBreakIntervalSec(), 600);
        QCOMPARE(s.shortBreakDurationSec(), 20);
        QCOMPARE(s.autostartEnabled(), false);

        // Reset back
        s.resetToDefaults();
        QCOMPARE(s.autostartEnabled(), true);
    }

    void testBreakControllerFlow() {
        Settings::instance().resetToDefaults();
        Settings::instance().setDndCheckEnabled(false);
        Settings::instance().setScreenLockCheckEnabled(false);
        // Setup rapid intervals for testing: 3s short break, 6s long break
        Settings::instance().setShortBreakIntervalSec(3);
        Settings::instance().setShortBreakDurationSec(2);
        Settings::instance().setLongBreakIntervalSec(6);
        Settings::instance().setLongBreakDurationSec(3);
        Settings::instance().setPreBreakNotificationSec(1);

        DndMonitor dnd;
        ScreenLockMonitor lock;
        BreakController controller(&dnd, &lock);

        QSignalSpy breakStartedSpy(&controller, &BreakController::breakStarted);
        QSignalSpy breakTickSpy(&controller, &BreakController::breakTick);
        QSignalSpy breakEndedSpy(&controller, &BreakController::breakEnded);
        QSignalSpy warningSpy(&controller, &BreakController::preBreakWarning);

        controller.start();
        QCOMPARE(controller.state(), BreakState::Running);

        // Wait for pre-break warning (after ~2 seconds)
        QTRY_VERIFY_WITH_TIMEOUT(warningSpy.count() >= 1, 3500);

        // Wait for short break to trigger (at ~3 seconds)
        QTRY_VERIFY_WITH_TIMEOUT(breakStartedSpy.count() >= 1, 3000);
        QCOMPARE(controller.isInBreak(), true);

        // Test Skip Break
        controller.skipBreak();
        QCOMPARE(controller.isInBreak(), false);
        QCOMPARE(breakEndedSpy.count(), 1);

        // Test Manual Trigger
        controller.triggerBreakNow(false);
        QCOMPARE(controller.isInBreak(), true);

        // Test Short Break Snooze
        controller.snoozeBreak(5);
        QCOMPARE(controller.isInBreak(), false);
        QCOMPARE(controller.state(), BreakState::Snoozed);
        QCOMPARE(controller.secondsUntilShortBreak(), 5);

        // Test Long Break Snooze: must snooze long break and NOT pull short break forward
        controller.triggerBreakNow(true);
        QCOMPARE(controller.isInBreak(), true);
        QCOMPARE(controller.isCurrentBreakLong(), true);
        controller.snoozeBreak(120);
        QCOMPARE(controller.isInBreak(), false);
        QCOMPARE(controller.state(), BreakState::Snoozed);
        QCOMPARE(controller.secondsUntilLongBreak(), 120);
        QVERIFY(controller.secondsUntilShortBreak() >= 120);

        controller.pause();
        QCOMPARE(controller.isPaused(), true);

        controller.resume();
        QCOMPARE(controller.isPaused(), false);

        Settings::instance().resetToDefaults();
    }

    void testAutostartHelper() {
        bool original = AutostartHelper::isAutostartEnabled();
        AutostartHelper::setAutostartEnabled(true);
        QCOMPARE(AutostartHelper::isAutostartEnabled(), true);

#ifndef Q_OS_WIN
        QString desktopPath = AutostartHelper::getAutostartFilePath();
        QFile f(desktopPath);
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = QString::fromUtf8(f.readAll());
        f.close();

        QVERIFY(content.contains("Icon=protecteye"));
        QVERIFY(content.contains("X-GNOME-Autostart-enabled=true"));
        QVERIFY(content.contains("X-KDE-autostart-after=panel"));
        QVERIFY(content.contains("X-MATE-Autostart-enabled=true"));
        QVERIFY(content.contains("X-XFCE-Autostart-enabled=true"));
#endif

        AutostartHelper::setAutostartEnabled(false);
        QCOMPARE(AutostartHelper::isAutostartEnabled(), false);

#ifndef Q_OS_WIN
        QFile fDisabled(desktopPath);
        QVERIFY(fDisabled.open(QIODevice::ReadOnly | QIODevice::Text));
        QString contentDisabled = QString::fromUtf8(fDisabled.readAll());
        fDisabled.close();

        QVERIFY(contentDisabled.contains("Hidden=true"));
        QVERIFY(contentDisabled.contains("X-GNOME-Autostart-enabled=false"));
#endif

        // Restore original
        AutostartHelper::setAutostartEnabled(original);
    }

    void testDesktopEnvironmentIntegration() {
        DndMonitor dnd;
        // isDndActive() should evaluate cleanly without crash across any desktop
        bool dndActive = dnd.isDndActive();
        Q_UNUSED(dndActive);

        ScreenLockMonitor lock;
        bool locked = lock.isLocked();
        Q_UNUSED(locked);
    }

    void testOverlayBadgeCentering() {
        OverlayWindow overlay(nullptr);
        overlay.resize(1920, 1080);

        auto* card = overlay.findChild<QWidget*>("cardWidget");
        QVERIFY(card != nullptr);
        auto* badge = overlay.findChild<QLabel*>("badgeLabel");
        QVERIFY(badge != nullptr);
        auto* title = overlay.findChild<QLabel*>("exerciseTitleLabel");
        QVERIFY(title != nullptr);

        const QStringList testBadges = {
            QStringLiteral("CONSCIOUS BLINKING"),
            QStringLiteral("👁️ KISA GÖZ MOLASI"),
            QStringLiteral("🌟 LONG BREAK"),
            QStringLiteral("NEAR-FAR FOCUS SHIFT"),
            QStringLiteral("REGISTRO DE SALUD VISUAL Y DESCANSO PROLONGADO")
        };

        for (const auto& text : testBadges) {
            ExerciseGuide guide;
            guide.badge = text;
            guide.title = QStringLiteral("Hydration & Eye Reset");
            guide.instruction = QStringLiteral("Blink slowly and fully several times.");
            guide.visualType = ExerciseVisualType::DistanceFocus;

            overlay.prepareBreak(false, 15, guide);
            overlay.show();
            qApp->processEvents();

            int cardCenterX = card->mapToGlobal(QPoint(0, 0)).x() + card->width() / 2;
            int badgeCenterX = badge->mapToGlobal(QPoint(0, 0)).x() + badge->width() / 2;
            int titleCenterX = title->mapToGlobal(QPoint(0, 0)).x() + title->width() / 2;

            QVERIFY2(std::abs(cardCenterX - badgeCenterX) <= 1,
                     qPrintable(QString("Badge is not centered! Card center: %1, Badge center: %2 for text '%3'")
                                .arg(cardCenterX).arg(badgeCenterX).arg(text)));

            QVERIFY2(std::abs(cardCenterX - titleCenterX) <= 1,
                     qPrintable(QString("Title is not centered! Card center: %1, Title center: %2")
                                .arg(cardCenterX).arg(titleCenterX)));
        }

        // Render the exact screenshot case (CONSCIOUS BLINKING) for visual verification
        ExerciseGuide consciousBlink;
        consciousBlink.badge = QStringLiteral("CONSCIOUS BLINKING");
        consciousBlink.title = QStringLiteral("Hydration & Eye Reset");
        consciousBlink.instruction = QStringLiteral("Blink slowly and fully several times to replenish the tear film and rest from monitor glare.");
        consciousBlink.visualType = ExerciseVisualType::DistanceFocus;
        overlay.prepareBreak(false, 11, consciousBlink);
        overlay.show();
        qApp->processEvents();
    }

    void testSnoozeAndSkipClick() {
        DndMonitor dnd;
        ScreenLockMonitor lock;
        BreakController controller(&dnd, &lock);
        OverlayManager overlayManager;

        QObject::connect(&controller, &BreakController::breakStarted,
                         &overlayManager, &OverlayManager::showBreak);
        QObject::connect(&controller, &BreakController::breakEnded,
                         &overlayManager, &OverlayManager::closeBreak);
        QObject::connect(&overlayManager, &OverlayManager::skipRequested,
                         &controller, &BreakController::skipBreak);
        QObject::connect(&overlayManager, &OverlayManager::snoozeRequested, [&controller]() {
            controller.snoozeBreak(120);
        });

        // 1. Trigger break
        controller.triggerBreakNow(false);
        QCOMPARE(controller.isInBreak(), true);
        qApp->processEvents();

        // 2. Find snooze button
        QPushButton* snoozeBtn = nullptr;
        QPushButton* skipBtn = nullptr;
        for (auto* w : QApplication::topLevelWidgets()) {
            auto* s = w->findChild<QPushButton*>("snoozeBtn");
            if (s && s->isEnabled()) snoozeBtn = s;
            auto* k = w->findChild<QPushButton*>("skipBtn");
            if (k && k->isEnabled()) skipBtn = k;
        }
        QVERIFY(snoozeBtn != nullptr);
        QVERIFY(skipBtn != nullptr);

        // 3. Click snooze button!
        snoozeBtn->click();
        qApp->processEvents();

        QCOMPARE(controller.isInBreak(), false);
        QCOMPARE(controller.state(), BreakState::Snoozed);
        QCOMPARE(controller.secondsUntilShortBreak(), 120);

        // 4. Trigger break again and test Skip
        controller.triggerBreakNow(false);
        QCOMPARE(controller.isInBreak(), true);
        qApp->processEvents();

        skipBtn = nullptr;
        for (auto* w : QApplication::topLevelWidgets()) {
            auto* k = w->findChild<QPushButton*>("skipBtn");
            if (k && k->isEnabled()) skipBtn = k;
        }
        QVERIFY(skipBtn != nullptr);

        skipBtn->click();
        qApp->processEvents();

        QCOMPARE(controller.isInBreak(), false);
        QCOMPARE(controller.state(), BreakState::Running);

        // 5. Test Long Break Snooze (ensures long break timer is also snoozed!)
        controller.triggerBreakNow(true);
        QCOMPARE(controller.isInBreak(), true);
        QCOMPARE(controller.isCurrentBreakLong(), true);
        qApp->processEvents();

        snoozeBtn = nullptr;
        for (auto* w : QApplication::topLevelWidgets()) {
            auto* s = w->findChild<QPushButton*>("snoozeBtn");
            if (s && s->isEnabled()) snoozeBtn = s;
        }
        QVERIFY(snoozeBtn != nullptr);

        snoozeBtn->click();
        qApp->processEvents();

        QCOMPARE(controller.isInBreak(), false);
        QCOMPARE(controller.state(), BreakState::Snoozed);
        QCOMPARE(controller.secondsUntilLongBreak(), 120);
        QCOMPARE(controller.secondsUntilShortBreak(), 120 + Settings::instance().shortBreakIntervalSec());
    }

    void testDndAndScreenLockInteractions() {
        Settings::instance().resetToDefaults();
        Settings::instance().setDndCheckEnabled(true);
        Settings::instance().setScreenLockCheckEnabled(true);

        MockDndMonitor dndMock;
        MockScreenLockMonitor lockMock;
        BreakController controller(&dndMock, &lockMock);

        // 1. Manual trigger MUST force break even when DND is active
        dndMock.mockDndActive = true;
        controller.triggerBreakNow(false);
        QCOMPARE(controller.isInBreak(), true);

        // 2. When screen is locked during break, overlay is safely closed and state paused
        lockMock.triggerLock(true);
        QCOMPARE(controller.isInBreak(), false);
        QCOMPARE(controller.isPaused(), true);

        // 3. When screen is unlocked, state resumes and both timers are refreshed
        lockMock.triggerLock(false);
        QCOMPARE(controller.isPaused(), false);
        QCOMPARE(controller.state(), BreakState::Running);
        QCOMPARE(controller.secondsUntilShortBreak(), Settings::instance().shortBreakIntervalSec());
        QCOMPARE(controller.secondsUntilLongBreak(), Settings::instance().longBreakIntervalSec());

        // 4. Natural break skipped silently when DND is active
        QSignalSpy dndSkipSpy(&controller, &BreakController::breakSilentlySkippedDnd);
        Settings::instance().setShortBreakIntervalSec(1);
        Settings::instance().setLongBreakIntervalSec(100);
        Settings::instance().save();
        dndMock.mockDndActive = true;
        controller.start();
        QTRY_VERIFY_WITH_TIMEOUT(dndSkipSpy.count() >= 1, 2500);
        QCOMPARE(controller.isInBreak(), false);

        // Reset settings back to defaults
        Settings::instance().resetToDefaults();
    }

    void testLocalization() {
        auto& loc = Localization::instance();

        // Arabic RTL test
        loc.setLanguage(AppLanguage::Arabic);
        QCOMPARE(loc.effectiveLanguageCode(), QStringLiteral("ar"));
        QCOMPARE(QGuiApplication::layoutDirection(), Qt::RightToLeft);

        // English test
        loc.setLanguage(AppLanguage::English);
        QCOMPARE(QGuiApplication::layoutDirection(), Qt::LeftToRight);
        QCOMPARE(loc.isTurkish(), false);
        QCOMPARE(loc.badgeShortBreak(), QStringLiteral("👁️ SHORT EYE BREAK"));
        QCOMPARE(loc.badgeLongBreak(), QStringLiteral("🌟 LONG BREAK"));
        QVERIFY(loc.shortBreakTips().size() >= 15);
        QVERIFY(loc.longBreakTips().size() >= 12);
        QVERIFY(!loc.getRandomTip(false).isEmpty());
        QVERIFY(!loc.getRandomTip(true).isEmpty());

        // Turkish test
        loc.setLanguage(AppLanguage::Turkish);
        QCOMPARE(loc.isTurkish(), true);
        QCOMPARE(loc.badgeShortBreak(), QStringLiteral("👁️ KISA GÖZ MOLASI"));
        QCOMPARE(loc.badgeLongBreak(), QStringLiteral("🌟 UZUN MOLA"));
        QVERIFY(loc.shortBreakTips().size() >= 15);
        QVERIFY(loc.longBreakTips().size() >= 12);
        QVERIFY(!loc.getRandomTip(false).isEmpty());
        QVERIFY(!loc.getRandomTip(true).isEmpty());

        // Available languages & dynamic catalog test
        auto langs = loc.availableLanguages();
        QVERIFY(langs.size() >= 2);
        bool hasEn = false, hasTr = false;
        for (const auto& l : langs) {
            if (l.code == QStringLiteral("en")) hasEn = true;
            if (l.code == QStringLiteral("tr")) hasTr = true;
        }
        QVERIFY(hasEn);
        QVERIFY(hasTr);

        // Generic get test & fallback test
        QCOMPARE(loc.get(QStringLiteral("app_name")), QStringLiteral("ProtectEye"));
        QCOMPARE(loc.get(QStringLiteral("nonexistent_key_xyz"), QStringLiteral("DEFAULT")), QStringLiteral("DEFAULT"));

        // Restore Auto
        loc.setLanguage(AppLanguage::Auto);
    }

    void testInteractiveExercisesSettings() {
        auto& s = Settings::instance();
        s.resetToDefaults();
        QCOMPARE(s.interactiveExercisesEnabled(), true);

        s.setInteractiveExercisesEnabled(false);
        QCOMPARE(s.interactiveExercisesEnabled(), false);
        s.save();

        s.load();
        QCOMPARE(s.interactiveExercisesEnabled(), false);

        s.resetToDefaults();
        QCOMPARE(s.interactiveExercisesEnabled(), true);
    }

    void testExerciseRotation() {
        auto& loc = Localization::instance();
        loc.setLanguage(AppLanguage::Turkish);

        auto shortList = loc.allShortBreakExercises();
        QCOMPARE(shortList.size(), 10);

        for (int i = 0; i < shortList.size() * 2; ++i) {
            auto ex = loc.getShortBreakExercise(i);
            QCOMPARE(ex.title, shortList.at(i % shortList.size()).title);
            QVERIFY(!ex.title.isEmpty());
            QVERIFY(!ex.instruction.isEmpty());
            QVERIFY(!ex.badge.isEmpty());
        }

        auto longList = loc.allLongBreakExercises();
        QCOMPARE(longList.size(), 8);

        for (int i = 0; i < longList.size() * 2; ++i) {
            auto ex = loc.getLongBreakExercise(i);
            QCOMPARE(ex.title, longList.at(i % longList.size()).title);
            QVERIFY(!ex.title.isEmpty());
            QVERIFY(!ex.instruction.isEmpty());
            QVERIFY(!ex.badge.isEmpty());
        }
    }

    void testExerciseLocalization() {
        auto& loc = Localization::instance();

        // Check English
        loc.setLanguage(AppLanguage::English);
        QCOMPARE(loc.breathInhale(), QStringLiteral("Inhale"));
        QCOMPARE(loc.breathExhale(), QStringLiteral("Exhale"));
        QCOMPARE(loc.breathHold(), QStringLiteral("Hold"));
        QCOMPARE(loc.squeezePhaseClose(), QStringLiteral("Close Eyes"));
        QCOMPARE(loc.squeezePhaseSqueeze(), QStringLiteral("Gently Squeeze"));
        QCOMPARE(loc.squeezePhaseOpen(), QStringLiteral("Open & Relax"));
        QVERIFY(loc.focusNearLabel().contains("Focus Near"));
        QVERIFY(loc.focusFarLabel().contains("Focus Far"));
        QVERIFY(loc.checkInteractiveExercises().contains("Interactive"));

        for (const auto& ex : loc.allShortBreakExercises()) {
            QVERIFY(!ex.badge.isEmpty());
            QVERIFY(!ex.title.isEmpty());
            QVERIFY(!ex.instruction.isEmpty());
        }
        for (const auto& ex : loc.allLongBreakExercises()) {
            QVERIFY(!ex.badge.isEmpty());
            QVERIFY(!ex.title.isEmpty());
            QVERIFY(!ex.instruction.isEmpty());
        }

        // Check Turkish
        loc.setLanguage(AppLanguage::Turkish);
        QCOMPARE(loc.breathInhale(), QStringLiteral("Nefes Al"));
        QCOMPARE(loc.breathExhale(), QStringLiteral("Nefes Ver"));
        QCOMPARE(loc.breathHold(), QStringLiteral("Nefesi Tut"));
        QCOMPARE(loc.squeezePhaseClose(), QStringLiteral("Kapat"));
        QCOMPARE(loc.squeezePhaseSqueeze(), QStringLiteral("Hafifçe Sık"));
        QCOMPARE(loc.squeezePhaseOpen(), QStringLiteral("Aç & Gevşet"));
        QVERIFY(loc.focusNearLabel().contains("Yakına Odaklan"));
        QVERIFY(loc.focusFarLabel().contains("Uzağa Odaklan"));
        QVERIFY(loc.checkInteractiveExercises().contains("İnteraktif"));

        for (const auto& ex : loc.allShortBreakExercises()) {
            QVERIFY(!ex.badge.isEmpty());
            QVERIFY(!ex.title.isEmpty());
            QVERIFY(!ex.instruction.isEmpty());
        }
        for (const auto& ex : loc.allLongBreakExercises()) {
            QVERIFY(!ex.badge.isEmpty());
            QVERIFY(!ex.title.isEmpty());
            QVERIFY(!ex.instruction.isEmpty());
        }

        loc.setLanguage(AppLanguage::Auto);
    }

    void testAllTopLanguages() {
        auto& loc = Localization::instance();
        const QStringList expectedCodes = {
            QStringLiteral("en"), QStringLiteral("tr"), QStringLiteral("es"),
            QStringLiteral("de"), QStringLiteral("fr"), QStringLiteral("pt"),
            QStringLiteral("ru"), QStringLiteral("zh"), QStringLiteral("ja"),
            QStringLiteral("ar"), QStringLiteral("hi")
        };

        const auto avail = loc.availableLanguages();
        QVERIFY(avail.size() >= 11);

        QStringList availCodes;
        for (const auto& info : avail) {
            availCodes.append(info.code);
            QVERIFY(!info.name.isEmpty());
        }

        for (const auto& code : expectedCodes) {
            QVERIFY2(availCodes.contains(code), qPrintable(QString("Missing language code in availableLanguages: %1").arg(code)));

            loc.setLanguageByCode(code);
            QCOMPARE(loc.effectiveLanguageCode(), code);
            QCOMPARE(loc.appName(), QStringLiteral("ProtectEye"));

            QVERIFY(!loc.trayTooltip().isEmpty());
            QVERIFY(!loc.badgeShortBreak().isEmpty());
            QVERIFY(!loc.badgeLongBreak().isEmpty());
            QVERIFY(!loc.buttonSkip().isEmpty());
            QVERIFY(!loc.settingsTitle().isEmpty());
            QVERIFY(!loc.settingsHeader().isEmpty());
            QVERIFY(!loc.breathInhale().isEmpty());
            QVERIFY(!loc.breathExhale().isEmpty());
            QVERIFY(!loc.breathHold().isEmpty());
            QVERIFY(!loc.squeezePhaseClose().isEmpty());
            QVERIFY(!loc.squeezePhaseSqueeze().isEmpty());
            QVERIFY(!loc.squeezePhaseOpen().isEmpty());
            QVERIFY(!loc.focusNearLabel().isEmpty());
            QVERIFY(!loc.focusFarLabel().isEmpty());

            QVERIFY(!loc.get(QStringLiteral("cli_description")).isEmpty());
            QVERIFY(!loc.get(QStringLiteral("cli_cmd_uninstall")).isEmpty());
            QVERIFY(!loc.get(QStringLiteral("cli_opt_test_break")).isEmpty());
            QVERIFY(!loc.get(QStringLiteral("cli_opt_test_warning")).isEmpty());
            QVERIFY(!loc.get(QStringLiteral("uninstall_running")).isEmpty());
            QVERIFY(!loc.get(QStringLiteral("uninstall_win_hint")).isEmpty());

            QCOMPARE(loc.shortBreakTips().size(), 16);
            QCOMPARE(loc.longBreakTips().size(), 14);
            QVERIFY(!loc.getRandomTip(false).isEmpty());
            QVERIFY(!loc.getRandomTip(true).isEmpty());

            const auto shortEx = loc.allShortBreakExercises();
            QCOMPARE(shortEx.size(), 10);
            for (const auto& ex : shortEx) {
                QVERIFY(!ex.badge.isEmpty());
                QVERIFY(!ex.title.isEmpty());
                QVERIFY(!ex.instruction.isEmpty());
            }

            const auto longEx = loc.allLongBreakExercises();
            QCOMPARE(longEx.size(), 8);
            for (const auto& ex : longEx) {
                QVERIFY(!ex.badge.isEmpty());
                QVERIFY(!ex.title.isEmpty());
                QVERIFY(!ex.instruction.isEmpty());
            }
        }

        // Reset to Auto
        loc.setLanguageByCode(QStringLiteral("auto"));
    }

    void testUpdateCheckerVersionComparison() {
        // Standard semver comparisons
        QCOMPARE(UpdateChecker::compareVersions(QStringLiteral("1.0.4"), QStringLiteral("1.0.3")), 1);
        QCOMPARE(UpdateChecker::compareVersions(QStringLiteral("1.0.3"), QStringLiteral("1.0.4")), -1);
        QCOMPARE(UpdateChecker::compareVersions(QStringLiteral("1.0.3"), QStringLiteral("1.0.3")), 0);

        // 'v' prefix handling
        QCOMPARE(UpdateChecker::compareVersions(QStringLiteral("v1.0.4"), QStringLiteral("1.0.3")), 1);
        QCOMPARE(UpdateChecker::compareVersions(QStringLiteral("1.0.3"), QStringLiteral("v1.0.3")), 0);
        QCOMPARE(UpdateChecker::compareVersions(QStringLiteral("v2.0.0"), QStringLiteral("v1.99.99")), 1);

        // Subversion and digit padding
        QCOMPARE(UpdateChecker::compareVersions(QStringLiteral("1.0"), QStringLiteral("1.0.0")), 0);
        QCOMPARE(UpdateChecker::compareVersions(QStringLiteral("1.0.3.1"), QStringLiteral("1.0.3")), 1);
        QCOMPARE(UpdateChecker::compareVersions(QStringLiteral("1.0.3"), QStringLiteral("1.0.3.1")), -1);

        // Suffix handling (-beta, -rc)
        QCOMPARE(UpdateChecker::compareVersions(QStringLiteral("v1.0.4-beta"), QStringLiteral("v1.0.3")), 1);

        // Current version check
        QVERIFY(!UpdateChecker::currentVersion().isEmpty());
        QCOMPARE(UpdateChecker::currentVersion(), QStringLiteral(PROTECTEYE_VERSION));
    }

    void testUpdateSettingsAndLocalization() {
        auto& s = Settings::instance();
        s.resetToDefaults();
        QCOMPARE(s.checkUpdatesEnabled(), true);

        s.setCheckUpdatesEnabled(false);
        QCOMPARE(s.checkUpdatesEnabled(), false);
        s.save();
        s.load();
        QCOMPARE(s.checkUpdatesEnabled(), false);

        s.resetToDefaults();
        QCOMPARE(s.checkUpdatesEnabled(), true);

        auto& loc = Localization::instance();
        const auto langs = loc.availableLanguages();
        for (const auto& lang : langs) {
            loc.setLanguageByCode(lang.code);
            QVERIFY(!loc.checkUpdates().isEmpty());
            QVERIFY(!loc.btnCheckUpdates().isEmpty());
            QVERIFY(!loc.updateStatusChecking().isEmpty());
            QVERIFY(!loc.updateStatusUpToDate().isEmpty());
            QVERIFY(!loc.updateStatusAvailable().isEmpty());
            QVERIFY(!loc.updateStatusFailed().isEmpty());
            QVERIFY(!loc.actionUpdateAvailable().isEmpty());
            QVERIFY(!loc.updateNotifTitle().isEmpty());
            QVERIFY(!loc.updateNotifBody().isEmpty());
        }

        loc.setLanguageByCode(QStringLiteral("auto"));
    }

    void testMultiScreenOverlayPlacement() {
        OverlayManager overlayManager;
        const auto screens = QGuiApplication::screens();
        QVERIFY(!screens.isEmpty());

        overlayManager.showBreak(false, 15);
        qApp->processEvents();

        int overlayCount = 0;
        const auto topWidgets = QApplication::topLevelWidgets();
        for (auto* w : topWidgets) {
            auto* overlay = qobject_cast<OverlayWindow*>(w);
            if (overlay && overlay->isVisible()) {
                overlayCount++;
                if (overlay->screen()) {
                    QVERIFY(screens.contains(overlay->screen()));
                }
            }
        }
        QCOMPARE(overlayCount, screens.size());

        overlayManager.updateCountdown(10, 15);
        qApp->processEvents();

        overlayManager.closeBreak();
        qApp->processEvents();

        int remainingVisible = 0;
        for (auto* w : QApplication::topLevelWidgets()) {
            auto* overlay = qobject_cast<OverlayWindow*>(w);
            if (overlay && overlay->isVisible()) {
                remainingVisible++;
            }
        }
        QCOMPARE(remainingVisible, 0);
    }

    void testSettingsDialogTabbedLayout() {
        SettingsDialog dlg;
        dlg.show();
        qApp->processEvents();

        QVERIFY(dlg.width() <= 600);
        QVERIFY(dlg.height() <= 480);

        auto* tabWidget = dlg.findChild<QTabWidget*>();
        QVERIFY(tabWidget != nullptr);
        QCOMPARE(tabWidget->count(), 3);

        // Tab 0: Schedules
        tabWidget->setCurrentIndex(0);
        qApp->processEvents();
        QPixmap p0 = dlg.grab();
        p0.save(QStringLiteral("/home/aa-linux/.gemini/antigravity/brain/0c4ec0a9-540e-45d4-9649-6f38da9ba228/settings_tab1_schedules.png"));

        // Tab 1: Features
        tabWidget->setCurrentIndex(1);
        qApp->processEvents();
        QPixmap p1 = dlg.grab();
        p1.save(QStringLiteral("/home/aa-linux/.gemini/antigravity/brain/0c4ec0a9-540e-45d4-9649-6f38da9ba228/settings_tab2_features.png"));

        // Tab 2: General & Updates
        tabWidget->setCurrentIndex(2);
        qApp->processEvents();
        QPixmap p2 = dlg.grab();
        p2.save(QStringLiteral("/home/aa-linux/.gemini/antigravity/brain/0c4ec0a9-540e-45d4-9649-6f38da9ba228/settings_tab3_general.png"));
    }
};


#include <QApplication>

int main(int argc, char *argv[]) {
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);
    TestProtectEye tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "test_protecteye.moc"

