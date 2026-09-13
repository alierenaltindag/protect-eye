#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include "core/BreakController.h"
#include "SettingsDialog.h"

class TrayManager : public QObject {
    Q_OBJECT

public:
    explicit TrayManager(BreakController* controller, QObject* parent = nullptr);
    ~TrayManager() override;

    QSystemTrayIcon* trayIcon() const { return m_trayIcon; }

    void showSettings();
    void notifyAlreadyRunning();

public slots:
    void onUpdateAvailable(const QString& version, const QString& releaseUrl, const QString& releaseNotes);

private slots:
    void onTick(int secToShort, int secToLong);
    void onStateChanged(BreakState newState);
    void onOpenSettings();
    void onOpenReleaseUrl();
    void retranslateUi();

private:
    void setupTray();
    void updateMenuText(int secToShort, int secToLong);
    QString formatTime(int totalSeconds) const;

    int m_lastSecToShort{0};
    int m_lastSecToLong{0};


    BreakController* m_controller;
    QSystemTrayIcon* m_trayIcon{nullptr};
    QMenu* m_menu{nullptr};

    QAction* m_statusAction{nullptr};
    QAction* m_updateAction{nullptr};
    QAction* m_shortStatusAction{nullptr};
    QAction* m_longStatusAction{nullptr};
    QAction* m_pauseResumeAction{nullptr};
    QAction* m_triggerShortAction{nullptr};
    QAction* m_triggerLongAction{nullptr};
    QAction* m_settingsAction{nullptr};
    QAction* m_quitAction{nullptr};

    QString m_latestReleaseUrl;
    QString m_latestVersion;

    SettingsDialog* m_settingsDialog{nullptr};
};
