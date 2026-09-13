#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QTimer>

class UpdateChecker : public QObject {
    Q_OBJECT

public:
    static UpdateChecker& instance();

    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker() override = default;

    static int compareVersions(const QString& v1, const QString& v2);
    static QString currentVersion();
    static int runCliUpdate(int argc, char* argv[]);

    void startBackgroundChecks();
    void checkForUpdates(bool manual = false, bool isStartup = false);

    bool isChecking() const { return m_isChecking; }
    QString latestVersion() const { return m_latestVersion; }
    QString releaseUrl() const { return m_releaseUrl; }

signals:
    void checkStarted();
    void updateAvailable(const QString& version, const QString& releaseUrl, const QString& releaseNotes);
    void checkFinished(bool updateFound, const QString& latestVersion, const QString& errorString);

private slots:
    void onPeriodicTimer();

private:
    void queryGithubApi(bool manual, bool isStartup);
    void queryFallbackVersion(bool manual, bool isStartup, const QString& primaryError);
    void processVersionResult(const QString& version, const QString& releaseUrl, const QString& releaseNotes, bool manual);

    QNetworkAccessManager* m_networkManager{nullptr};
    QTimer* m_periodicTimer{nullptr};
    bool m_isChecking{false};
    QString m_latestVersion;
    QString m_releaseUrl;
};
