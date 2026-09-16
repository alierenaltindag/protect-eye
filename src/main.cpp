#include <QApplication>
#include <QIcon>
#include <QDebug>
#include <QCommandLineParser>
#include <QFile>
#include <cstdlib>

#include "core/Settings.h"
#include "core/Localization.h"
#include "core/BreakController.h"
#include "services/DndMonitor.h"
#include "services/ScreenLockMonitor.h"
#include "services/IdleMonitor.h"
#include "services/SoundManager.h"
#include "services/NotificationService.h"
#include "services/UpdateChecker.h"
#include "ui/OverlayManager.h"
#include "ui/TrayManager.h"
#include "utils/SingleInstanceGuard.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    QApplication::setApplicationName(QStringLiteral("ProtectEye"));
    QApplication::setApplicationVersion(QStringLiteral(PROTECTEYE_VERSION));
    QApplication::setOrganizationName(QStringLiteral("ProtectEye"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/app_icon.svg")));
    QApplication::setQuitOnLastWindowClosed(false);

    // Initialize settings and configured language early
    Settings::instance();
    auto& loc = Localization::instance();
    QApplication::setApplicationDisplayName(loc.trayTooltip());

    // Handle "protecteye uninstall" and "protecteye update" before Qt argument parsing
    if (argc >= 2 && QString::fromUtf8(argv[1]) == QStringLiteral("uninstall")) {
#ifdef Q_OS_WIN
        QString uninstPath = QCoreApplication::applicationDirPath() + QStringLiteral("/unins000.exe");
        if (QFile::exists(uninstPath)) {
            qInfo().noquote() << loc.get(
                QStringLiteral("uninstall_running"),
                QStringLiteral("Running ProtectEye uninstaller...")
            );
            return std::system(qPrintable(QStringLiteral("\"\"%1\"\"").arg(uninstPath)));
        }
        qWarning().noquote() << loc.get(
            QStringLiteral("uninstall_win_hint"),
            QStringLiteral("On Windows, uninstall via Settings > Installed Apps or Start Menu shortcut.")
        );
        return 1;
#else
        qInfo().noquote() << loc.get(
            QStringLiteral("uninstall_running"),
            QStringLiteral("Running ProtectEye uninstaller...")
        );
        QStringList candidates = {
            QCoreApplication::applicationDirPath() + "/uninstall.sh",
            QCoreApplication::applicationDirPath() + "/../uninstall.sh",
            QStringLiteral("/usr/local/share/protecteye/uninstall.sh"),
            QStringLiteral("/usr/share/protecteye/uninstall.sh")
        };
        QString foundScript;
        for (const auto& path : candidates) {
            if (QFile::exists(path)) {
                foundScript = path;
                break;
            }
        }
        if (!foundScript.isEmpty()) {
            return std::system(qPrintable(QStringLiteral("bash \"%1\"").arg(foundScript)));
        }
        // Fallback to repository uninstaller if no local installation script was found
        int ret = std::system(
            "bash -c \"$(curl -fsSL https://raw.githubusercontent.com/alierenaltindag/protect-eye/main/uninstall.sh)\""
        );
        return ret;
#endif
    }

    if (argc >= 2 && (QString::fromUtf8(argv[1]) == QStringLiteral("update") ||
                      QString::fromUtf8(argv[1]) == QStringLiteral("--update"))) {
        return UpdateChecker::runCliUpdate(argc, argv);
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(
        loc.get(
            QStringLiteral("cli_description"),
            QStringLiteral("Cross-platform eye health and break assistant.\n\n"
                           "Commands:\n"
                           "  update              Checks and automatically updates ProtectEye to the latest version\n"
                           "  uninstall           Completely uninstalls ProtectEye from your system")
        )
    );
    parser.addPositionalArgument(
        QStringLiteral("command"),
        loc.get(
            QStringLiteral("cli_cmd_update"),
            QStringLiteral("Optional command: 'update' to update, or 'uninstall' to remove ProtectEye")
        ),
        QStringLiteral("[command]")
    );
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption testBreakOption(
        QStringList() << "t" << "test-break",
        loc.get(
            QStringLiteral("cli_opt_test_break"),
            QStringLiteral("Triggers a test break overlay immediately on startup")
        )
    );
    parser.addOption(testBreakOption);

    QCommandLineOption testWarningOption(
        QStringList() << "w" << "test-warning",
        loc.get(
            QStringLiteral("cli_opt_test_warning"),
            QStringLiteral("Triggers a test pre-break warning notification immediately on startup")
        )
    );
    parser.addOption(testWarningOption);

    QCommandLineOption autostartOption(
        QStringList() << "autostart",
        loc.get(
            QStringLiteral("cli_opt_autostart"),
            QStringLiteral("System startup autostart mode flag")
        )
    );
    parser.addOption(autostartOption);

    parser.process(app);

    // If launched via system autostart but user disabled autostart in settings, exit cleanly immediately
    if (parser.isSet(autostartOption) && !Settings::instance().autostartEnabled()) {
        qInfo().noquote() << loc.get(
            QStringLiteral("autostart_disabled_exit"),
            QStringLiteral("ProtectEye autostart is disabled in user settings. Exiting...")
        );
        return 0;
    }

    // Single-instance protection
    SingleInstanceGuard singleInstanceGuard;
    if (!singleInstanceGuard.tryRun()) {
        qInfo().noquote() << loc.alreadyRunningMessage();
        return 0;
    }

    // Core Services
    DndMonitor dndMonitor;
    ScreenLockMonitor lockMonitor;
    IdleMonitor idleMonitor;
    SoundManager soundManager;
    BreakController breakController(&dndMonitor, &lockMonitor, &idleMonitor);
    OverlayManager overlayManager;
    TrayManager trayManager(&breakController);
    NotificationService notificationService(trayManager.trayIcon());

    // Connect Single-Instance Activation (bring settings to front and notify)
    QObject::connect(&singleInstanceGuard, &SingleInstanceGuard::activateRequested,
                     &trayManager, [&trayManager, &notificationService, &loc]() {
        trayManager.showSettings();
        notificationService.showCustomNotification(loc.alreadyRunningTitle(), loc.alreadyRunningMessage());
    });

    // Connect Notifications
    QObject::connect(&breakController, &BreakController::preBreakWarning,
                     &notificationService, &NotificationService::showPreBreakWarning);

    // Connect Overlays & Audio
    QObject::connect(&breakController, &BreakController::breakStarted,
                     &overlayManager, &OverlayManager::showBreak);
    QObject::connect(&breakController, &BreakController::breakStarted,
                     &soundManager, &SoundManager::playBreakStart);

    QObject::connect(&breakController, &BreakController::breakTick,
                     &overlayManager, &OverlayManager::updateCountdown);

    QObject::connect(&breakController, &BreakController::breakEnded,
                     &overlayManager, &OverlayManager::closeBreak);
    QObject::connect(&breakController, &BreakController::breakCompleted,
                     &soundManager, &SoundManager::playBreakEnd);

    // Overlay user interactions -> Controller
    QObject::connect(&overlayManager, &OverlayManager::skipRequested,
                     &breakController, &BreakController::skipBreak);
    QObject::connect(&overlayManager, &OverlayManager::snoozeRequested,
                     &breakController, [&breakController]() {
        breakController.snoozeBreak(Settings::instance().snoozeDurationSec());
    });

    // Update Checker
    auto& updateChecker = UpdateChecker::instance();
    QObject::connect(&updateChecker, &UpdateChecker::updateAvailable,
                     &trayManager, &TrayManager::onUpdateAvailable);
    QObject::connect(&updateChecker, &UpdateChecker::updateAvailable,
                     &notificationService, &NotificationService::showUpdateNotification);
    updateChecker.startBackgroundChecks();

    // Start controller
    breakController.start();

    if (parser.isSet(testWarningOption)) {
        qDebug() << "Test warning option detected, triggering pre-break notification...";
        notificationService.showPreBreakWarning(false, 30);
    }

    if (parser.isSet(testBreakOption)) {
        qDebug() << "Test break option detected, triggering break...";
        breakController.triggerBreakNow(false);
    }

    return app.exec();
}

