#include <QTest>
#include <QSignalSpy>
#include "core/Settings.h"
#include "core/Localization.h"
#include "core/BreakController.h"

#include "services/DndMonitor.h"
#include "services/ScreenLockMonitor.h"
#include "services/IdleMonitor.h"
#include "services/UpdateChecker.h"
#include "services/NotificationService.h"
#include "utils/AutostartHelper.h"
#include "ui/OverlayWindow.h"
#include "ui/OverlayManager.h"
#include "ui/EyeExerciseWidget.h"
#include "ui/SettingsDialog.h"
#include <QAbstractItemView>
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

class MockIdleMonitor : public IdleMonitor {
public:
    int mockIdleSeconds{0};
    bool mockMediaPlaying{false};

    int getIdleSeconds() const override { return mockIdleSeconds; }
    bool isMediaPlaying() const override { return mockMediaPlaying; }
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
        QCOMPARE(s.idleCheckEnabled(), true);
        QCOMPARE(s.idleThresholdSec(), 180);
        QCOMPARE(s.idleResetThresholdSec(), 300);
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
        QSignalSpy warningSpy(&controller, &BreakController::preBreakWarning);
        Settings::instance().setShortBreakIntervalSec(2);
        Settings::instance().setPreBreakNotificationSec(1);
        Settings::instance().setLongBreakIntervalSec(100);
        Settings::instance().save();
        dndMock.mockDndActive = true;
        controller.start();
        QTRY_VERIFY_WITH_TIMEOUT(dndSkipSpy.count() >= 1, 3500);
        QCOMPARE(controller.isInBreak(), false);
        // Pre-break warning must be suppressed while DND is active
        QCOMPARE(warningSpy.count(), 0);

        // Reset settings back to defaults
        Settings::instance().resetToDefaults();
    }

    void testAutostartCrossPlatform() {
#ifndef Q_OS_WIN
        QString desktopPath = AutostartHelper::getAutostartFilePath();
        QVERIFY(!desktopPath.isEmpty());
        QVERIFY(desktopPath.endsWith(QStringLiteral("protecteye.desktop")));

        // Test Enable
        AutostartHelper::setAutostartEnabled(true);
        QVERIFY(QFile::exists(desktopPath));
        QVERIFY(AutostartHelper::isAutostartEnabled());

        QFile file(desktopPath);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = QString::fromUtf8(file.readAll());
        file.close();

        QVERIFY(content.contains(QStringLiteral("Type=Application")));
        QVERIFY(content.contains(QStringLiteral("Exec=")));
        QVERIFY(content.contains(QStringLiteral("--autostart")));
        QVERIFY(content.contains(QStringLiteral("TryExec=")));
        QVERIFY(content.contains(QStringLiteral("X-GNOME-Autostart-enabled=true")));
        QVERIFY(content.contains(QStringLiteral("X-KDE-autostart-after=panel")));
        QVERIFY(content.contains(QStringLiteral("X-MATE-Autostart-enabled=true")));
        QVERIFY(content.contains(QStringLiteral("X-XFCE-Autostart-enabled=true")));
        QVERIFY(!content.contains(QStringLiteral("Hidden=true")));

        // Test Disable
        AutostartHelper::setAutostartEnabled(false);
        QVERIFY(!AutostartHelper::isAutostartEnabled());

        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString disabledContent = QString::fromUtf8(file.readAll());
        file.close();

        QVERIFY(disabledContent.contains(QStringLiteral("Hidden=true")));
        QVERIFY(disabledContent.contains(QStringLiteral("X-GNOME-Autostart-enabled=false")));
        QVERIFY(disabledContent.contains(QStringLiteral("X-MATE-Autostart-enabled=false")));
        QVERIFY(disabledContent.contains(QStringLiteral("X-XFCE-Autostart-enabled=false")));

        // Clean up test file
        QFile::remove(desktopPath);
#else
        AutostartHelper::setAutostartEnabled(true);
        QVERIFY(AutostartHelper::isAutostartEnabled());
        AutostartHelper::setAutostartEnabled(false);
        QVERIFY(!AutostartHelper::isAutostartEnabled());
#endif
    }

    void testDndMonitorRealSystem() {
        DndMonitor monitor;
        // Verify call executes cleanly without crash or infinite hang on current desktop
        bool active = monitor.isDndActive();
        Q_UNUSED(active);
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
            QVERIFY(!loc.preBreakTitle(false).isEmpty());
            QVERIFY(!loc.preBreakTitle(true).isEmpty());
            QVERIFY(!loc.preBreakMessage(30).isEmpty());
            QVERIFY(!loc.updateNotifTitle().isEmpty());
            QVERIFY(!loc.updateNotifBody().arg(QStringLiteral("1.0.8")).isEmpty());
            QVERIFY(!loc.actionUpdateAvailable().arg(QStringLiteral("1.0.8")).isEmpty());
            QVERIFY(!loc.alreadyRunningTitle().isEmpty());
            QVERIFY(!loc.alreadyRunningMessage().isEmpty());
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

    void testNotificationServiceUpdateNotification() {
        QSystemTrayIcon tray;
        NotificationService notif(&tray);
        // Valid version and URL
        notif.showUpdateNotification(QStringLiteral("1.0.8"), QStringLiteral("https://github.com/alierenaltindag/protect-eye/releases/tag/v1.0.8"));
        // Empty URL fallback
        notif.showUpdateNotification(QStringLiteral("1.0.8"), QString());
    }

    void testUpdateCheckerStartupAndFallback() {
        auto& checker = UpdateChecker::instance();
        QSignalSpy startedSpy(&checker, &UpdateChecker::checkStarted);
        checker.checkForUpdates(false, /* isStartup = */ true);
        QCOMPARE(startedSpy.count(), 1);
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

    void testInteractiveExercisesToggleOnOverlay() {
        auto& s = Settings::instance();
        s.resetToDefaults();
        s.setLanguage("tr");
        Localization::instance().setLanguageByCode("tr");

        // 1. With interactive exercises ENABLED (default):
        s.setInteractiveExercisesEnabled(true);
        OverlayManager manager1;
        manager1.showBreak(false, 20);
        qApp->processEvents();

        OverlayWindow* activeOverlay1 = nullptr;
        for (auto* w : QApplication::topLevelWidgets()) {
            auto* overlay = qobject_cast<OverlayWindow*>(w);
            if (overlay && overlay->isVisible()) {
                activeOverlay1 = overlay;
                break;
            }
        }
        QVERIFY(activeOverlay1 != nullptr);

        auto* titleLabel1 = activeOverlay1->findChild<QLabel*>("exerciseTitleLabel");
        auto* exerciseWidget1 = activeOverlay1->findChild<EyeExerciseWidget*>("exerciseWidget");
        auto* tipLabel1 = activeOverlay1->findChild<QLabel*>("tipLabel");
        auto* badgeLabel1 = activeOverlay1->findChild<QLabel*>("badgeLabel");

        QVERIFY(titleLabel1 != nullptr);
        QVERIFY(exerciseWidget1 != nullptr);
        QVERIFY(tipLabel1 != nullptr);
        QVERIFY(badgeLabel1 != nullptr);

        QVERIFY(titleLabel1->isVisible());
        QVERIFY(!titleLabel1->text().isEmpty());
        QVERIFY(exerciseWidget1->isVisible());

        manager1.closeBreak();
        qApp->processEvents();

        // 2. With interactive exercises DISABLED:
        s.setInteractiveExercisesEnabled(false);
        OverlayManager manager2;
        manager2.showBreak(false, 20);
        qApp->processEvents();

        OverlayWindow* activeOverlay2 = nullptr;
        for (auto* w : QApplication::topLevelWidgets()) {
            auto* overlay = qobject_cast<OverlayWindow*>(w);
            if (overlay && overlay->isVisible()) {
                activeOverlay2 = overlay;
                break;
            }
        }
        QVERIFY(activeOverlay2 != nullptr);

        auto* titleLabel2 = activeOverlay2->findChild<QLabel*>("exerciseTitleLabel");
        auto* exerciseWidget2 = activeOverlay2->findChild<EyeExerciseWidget*>("exerciseWidget");
        auto* tipLabel2 = activeOverlay2->findChild<QLabel*>("tipLabel");
        auto* badgeLabel2 = activeOverlay2->findChild<QLabel*>("badgeLabel");

        QVERIFY(titleLabel2 != nullptr);
        QVERIFY(exerciseWidget2 != nullptr);
        QVERIFY(tipLabel2 != nullptr);
        QVERIFY(badgeLabel2 != nullptr);

        // Verification of disabled state:
        // - Badge must be standard short break badge ("👁️ KISA GÖZ MOLASI"), not "İNTERAKTİF GÖZ TAKİBİ"
        QCOMPARE(badgeLabel2->text(), Localization::instance().badgeShortBreak());
        QVERIFY(!badgeLabel2->text().contains("İNTERAKTİF"));
        QVERIFY(!badgeLabel2->text().contains("INTERACTIVE"));

        // - Title must be hidden
        QVERIFY(!titleLabel2->isVisible());

        // - Exercise widget animation must be hidden
        QVERIFY(!exerciseWidget2->isVisible());

        // - Tip must NOT tell user to follow glowing blue lights that don't exist
        QVERIFY(!tipLabel2->text().contains("parlayan mavi ışığı"));
        QVERIFY(!tipLabel2->text().contains("mavi ışık"));
        // - Tip must be a valid tip from shortBreakTips
        const auto validTips = Localization::instance().shortBreakTips();
        QVERIFY(validTips.contains(tipLabel2->text()));

        // Capture screenshot of clean disabled state
        QPixmap pix = activeOverlay2->grab();
        pix.save(QStringLiteral("/home/aa-linux/.gemini/antigravity/brain/0c4ec0a9-540e-45d4-9649-6f38da9ba228/overlay_break_exercises_disabled.png"));

        manager2.closeBreak();
        qApp->processEvents();

        s.resetToDefaults();
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

        // Test and capture ComboBox popup
        auto* langCombo = dlg.findChild<QComboBox*>();
        QVERIFY(langCombo != nullptr);
        langCombo->showPopup();
        qApp->processEvents();
        if (langCombo->view() && langCombo->view()->window()) {
            QPixmap pPopup = langCombo->view()->window()->grab();
            pPopup.save(QStringLiteral("/home/aa-linux/.gemini/antigravity/brain/0c4ec0a9-540e-45d4-9649-6f38da9ba228/settings_lang_dropdown.png"));
        }
        langCombo->hidePopup();
    }

    void testOverlayFullScreenRetention() {
        QScreen* primary = QGuiApplication::primaryScreen();
        OverlayWindow overlay(primary);
        overlay.prepareBreak(false, 20, Localization::instance().getShortBreakExercise(0));
        overlay.present(true);
        qApp->processEvents();

        QVERIFY(overlay.isFullScreen());
        QVERIFY(overlay.windowState() & Qt::WindowFullScreen);

        // Simulate a showEvent or window update to ensure it does not revert to WindowNoState
        QShowEvent showEv;
        QApplication::sendEvent(&overlay, &showEv);
        qApp->processEvents();

        QVERIFY(overlay.isFullScreen());
        QVERIFY(overlay.windowState() & Qt::WindowFullScreen);
    }

    void testIdleDetectionPausesTimers() {
        Settings::instance().resetToDefaults();
        Settings::instance().setIdleCheckEnabled(true);
        Settings::instance().setIdleThresholdSec(2);
        Settings::instance().setShortBreakIntervalSec(60);
        Settings::instance().setLongBreakIntervalSec(120);

        DndMonitor dnd;
        ScreenLockMonitor lock;
        MockIdleMonitor idle;
        BreakController controller(&dnd, &lock, &idle);

        controller.start();
        QCOMPARE(controller.state(), BreakState::Running);

        idle.mockIdleSeconds = 0;
        idle.mockMediaPlaying = false;
        QTest::qWait(1100);
        int secAfterActive = controller.secondsUntilShortBreak();
        QVERIFY(secAfterActive < 60);

        // Now become idle (> 2s)
        idle.mockIdleSeconds = 5;
        QTest::qWait(1100);
        QVERIFY(controller.isUserIdle());
        int secWhenIdle = controller.secondsUntilShortBreak();

        // Wait another second while idle - timer should NOT decrement
        QTest::qWait(1100);
        QCOMPARE(controller.secondsUntilShortBreak(), secWhenIdle);
    }

    void testIdleWithMediaPlayingDoesNotPause() {
        Settings::instance().resetToDefaults();
        Settings::instance().setIdleCheckEnabled(true);
        Settings::instance().setIdleThresholdSec(2);
        Settings::instance().setShortBreakIntervalSec(60);

        DndMonitor dnd;
        ScreenLockMonitor lock;
        MockIdleMonitor idle;
        BreakController controller(&dnd, &lock, &idle);

        controller.start();

        // Inactive for 10s, BUT media is playing (video/audio)
        idle.mockIdleSeconds = 10;
        idle.mockMediaPlaying = true;

        QTest::qWait(1100);
        QCOMPARE(controller.isUserIdle(), false);
        int sec1 = controller.secondsUntilShortBreak();

        QTest::qWait(1100);
        int sec2 = controller.secondsUntilShortBreak();
        QVERIFY(sec2 < sec1);
    }

    void testUserReturnShortIdleResumes() {
        Settings::instance().resetToDefaults();
        Settings::instance().setIdleCheckEnabled(true);
        Settings::instance().setIdleThresholdSec(2);
        Settings::instance().setIdleResetThresholdSec(30);
        Settings::instance().setShortBreakIntervalSec(60);

        DndMonitor dnd;
        ScreenLockMonitor lock;
        MockIdleMonitor idle;
        BreakController controller(&dnd, &lock, &idle);

        controller.start();

        // Become idle
        idle.mockIdleSeconds = 5;
        idle.mockMediaPlaying = false;
        QTest::qWait(1100);
        QVERIFY(controller.isUserIdle());
        int pausedSec = controller.secondsUntilShortBreak();

        // Return quickly (< 30s)
        idle.mockIdleSeconds = 0;
        QTest::qWait(1100);
        QCOMPARE(controller.isUserIdle(), false);
        QVERIFY(controller.secondsUntilShortBreak() < 60);
        QVERIFY(controller.secondsUntilShortBreak() <= pausedSec);
    }

    void testUserReturnLongIdleResets() {
        Settings::instance().resetToDefaults();
        Settings::instance().setIdleCheckEnabled(true);
        Settings::instance().setIdleThresholdSec(1);
        Settings::instance().setIdleResetThresholdSec(1);
        Settings::instance().setShortBreakIntervalSec(60);
        Settings::instance().setLongBreakIntervalSec(120);

        DndMonitor dnd;
        ScreenLockMonitor lock;
        MockIdleMonitor idle;
        BreakController controller(&dnd, &lock, &idle);

        controller.start();

        // Let it count down 2 seconds
        idle.mockIdleSeconds = 0;
        QTest::qWait(1100);
        QTest::qWait(1100);
        QVERIFY(controller.secondsUntilShortBreak() < 60);

        // Become idle
        idle.mockIdleSeconds = 5;
        idle.mockMediaPlaying = false;
        QTest::qWait(1100);
        QVERIFY(controller.isUserIdle());

        // Wait 1.5 seconds so idleSec >= 1s reset threshold
        QTest::qWait(1500);

        // User returns!
        idle.mockIdleSeconds = 0;
        QTest::qWait(1100);
        QCOMPARE(controller.isUserIdle(), false);

        // Should be reset to fresh 60s and 120s!
        QCOMPARE(controller.secondsUntilShortBreak(), 60);
        QCOMPARE(controller.secondsUntilLongBreak(), 120);
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

