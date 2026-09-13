#include "TrayManager.h"
#include "core/Localization.h"
#include <QApplication>
#include <QCursor>
#include <QIcon>
#include <QTimer>
#include <QDesktopServices>
#include <QUrl>

TrayManager::TrayManager(BreakController* controller, QObject* parent)
    : QObject(parent)
    , m_controller(controller) {
    setupTray();

    connect(m_controller, &BreakController::tick, this, &TrayManager::onTick);
    connect(m_controller, &BreakController::stateChanged, this, &TrayManager::onStateChanged);
    connect(&Localization::instance(), &Localization::languageChanged, this, &TrayManager::retranslateUi);
}

TrayManager::~TrayManager() {
    delete m_settingsDialog;
    delete m_menu;
}

void TrayManager::setupTray() {
    auto& loc = Localization::instance();

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(QStringLiteral(":/tray_active.svg")));
    m_trayIcon->setToolTip(loc.trayTooltip());

    m_menu = new QMenu();
    m_menu->setStyleSheet(R"(
        QMenu {
            background-color: #0f172a;
            color: #f8fafc;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 6px;
        }
        QMenu::item {
            padding: 8px 24px;
            border-radius: 6px;
        }
        QMenu::item:selected {
            background-color: #1e293b;
            color: #38bdf8;
        }
        QMenu::separator {
            height: 1px;
            background: #334155;
            margin: 6px 12px;
        }
    )");

    m_updateAction = m_menu->addAction(QString());
    m_updateAction->setVisible(false);
    connect(m_updateAction, &QAction::triggered, this, &TrayManager::onOpenReleaseUrl);

    m_statusAction = m_menu->addAction(loc.statusActive());
    m_statusAction->setEnabled(false);

    m_shortStatusAction = m_menu->addAction(loc.shortBreakLabel(QStringLiteral("--:--")));
    m_shortStatusAction->setEnabled(false);

    m_longStatusAction = m_menu->addAction(loc.longBreakLabel(QStringLiteral("--:--")));
    m_longStatusAction->setEnabled(false);

    m_menu->addSeparator();

    m_pauseResumeAction = m_menu->addAction(loc.actionPause(), m_controller, &BreakController::togglePause);
    
    m_triggerShortAction = m_menu->addAction(loc.actionTakeShortNow(), [this]() {
        m_controller->triggerBreakNow(false);
    });

    m_triggerLongAction = m_menu->addAction(loc.actionTakeLongNow(), [this]() {
        m_controller->triggerBreakNow(true);
    });

    m_menu->addSeparator();

    m_settingsAction = m_menu->addAction(loc.actionSettings(), this, &TrayManager::onOpenSettings);
    m_quitAction = m_menu->addAction(loc.actionQuit(), qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_menu);
    m_trayIcon->show();

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "[ProtectEye] System tray is not currently detected in this desktop environment."
                   << "If using GNOME, please ensure 'gnome-shell-extension-appindicator' is installed.";
        // Some desktop environments (or autostart sequences) register the tray daemon shortly after launch
        QTimer::singleShot(2500, this, [this]() {
            if (QSystemTrayIcon::isSystemTrayAvailable() && !m_trayIcon->isVisible()) {
                m_trayIcon->show();
            }
        });
    }

    // Left-click opens the context menu (same as right-click)
    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            m_menu->popup(QCursor::pos());
        }
    });

    // Show startup notification
#ifdef Q_OS_WIN
    constexpr int startupNotifDelay = 2000;
#else
    constexpr int startupNotifDelay = 1200;
#endif
    QTimer::singleShot(startupNotifDelay, m_trayIcon, [this]() {
        auto& l = Localization::instance();
        m_trayIcon->showMessage(
            l.startupNotificationTitle(),
            l.startupNotificationMessage(),
            QSystemTrayIcon::Information,
            4000
        );
    });
}

void TrayManager::retranslateUi() {
    auto& loc = Localization::instance();

    m_triggerShortAction->setText(loc.actionTakeShortNow());
    m_triggerLongAction->setText(loc.actionTakeLongNow());
    m_settingsAction->setText(loc.actionSettings());
    m_quitAction->setText(loc.actionQuit());

    if (m_updateAction && m_updateAction->isVisible() && !m_latestVersion.isEmpty()) {
        m_updateAction->setText(loc.actionUpdateAvailable().arg(m_latestVersion));
    }

    onStateChanged(m_controller->state());
    updateMenuText(m_lastSecToShort, m_lastSecToLong);
}

QString TrayManager::formatTime(int totalSeconds) const {
    if (totalSeconds < 0) totalSeconds = 0;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

void TrayManager::onTick(int secToShort, int secToLong) {
    m_lastSecToShort = secToShort;
    m_lastSecToLong = secToLong;
    updateMenuText(secToShort, secToLong);
}

void TrayManager::updateMenuText(int secToShort, int secToLong) {
    auto& loc = Localization::instance();

    if (m_controller->isPaused()) {
        m_trayIcon->setToolTip(loc.appName() + QStringLiteral(" - ") + loc.statusPaused());
        m_statusAction->setText(loc.statusPaused());
        return;
    }

    QString shortText = loc.shortBreakLabel(formatTime(secToShort));
    QString longText = loc.longBreakLabel(formatTime(secToLong));

    m_shortStatusAction->setText(shortText);
    m_longStatusAction->setText(longText);

    if (m_controller->state() == BreakState::Snoozed) {
        m_statusAction->setText(loc.statusSnoozed());
        m_trayIcon->setToolTip(QStringLiteral("%1 (%2)\n%3\n%4")
                                   .arg(loc.appName(), loc.statusSnoozed(), shortText, longText));
    } else {
        m_trayIcon->setToolTip(QStringLiteral("%1\n%2\n%3").arg(loc.appName(), shortText, longText));
    }
}

void TrayManager::onStateChanged(BreakState newState) {
    auto& loc = Localization::instance();

    switch (newState) {
    case BreakState::Running:
        m_trayIcon->setIcon(QIcon(QStringLiteral(":/tray_active.svg")));
        m_pauseResumeAction->setText(loc.actionPause());
        m_statusAction->setText(loc.statusActive());
        break;
    case BreakState::Paused:
        m_trayIcon->setIcon(QIcon(QStringLiteral(":/tray_paused.svg")));
        m_pauseResumeAction->setText(loc.actionResume());
        m_statusAction->setText(loc.statusPaused());
        break;
    case BreakState::InShortBreak:
    case BreakState::InLongBreak:
        m_trayIcon->setIcon(QIcon(QStringLiteral(":/tray_break.svg")));
        m_pauseResumeAction->setText(loc.actionPause());
        m_statusAction->setText(loc.statusInBreak());
        break;
    case BreakState::Snoozed:
        m_trayIcon->setIcon(QIcon(QStringLiteral(":/tray_active.svg")));
        m_pauseResumeAction->setText(loc.actionPause());
        m_statusAction->setText(loc.statusSnoozed());
        break;
    }
}

void TrayManager::onOpenSettings() {
    showSettings();
}

void TrayManager::showSettings() {
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog();
    }
    m_settingsDialog->show();
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
}

void TrayManager::notifyAlreadyRunning() {
    if (m_trayIcon) {
        auto& loc = Localization::instance();
        m_trayIcon->showMessage(
            loc.alreadyRunningTitle(),
            loc.alreadyRunningMessage(),
            QSystemTrayIcon::Information,
            4000
        );
    }
}

void TrayManager::onUpdateAvailable(const QString& version, const QString& releaseUrl, const QString& releaseNotes) {
    Q_UNUSED(releaseNotes);
    m_latestVersion = version;
    m_latestReleaseUrl = releaseUrl;

    auto& loc = Localization::instance();
    m_updateAction->setText(loc.actionUpdateAvailable().arg(version));
    m_updateAction->setVisible(true);
}

void TrayManager::onOpenReleaseUrl() {
    QString url = m_latestReleaseUrl.isEmpty() 
        ? QStringLiteral("https://github.com/alierenaltindag/protect-eye/releases/latest") 
        : m_latestReleaseUrl;
    QDesktopServices::openUrl(QUrl(url));
}

