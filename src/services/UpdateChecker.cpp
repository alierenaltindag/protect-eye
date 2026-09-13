#include "UpdateChecker.h"
#include "core/Settings.h"
#include "core/Localization.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QRegularExpression>
#include <QSslError>
#include <cstdlib>
#include <algorithm>

UpdateChecker& UpdateChecker::instance() {
    static UpdateChecker s_instance;
    return s_instance;
}

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_periodicTimer(new QTimer(this)) {
    connect(m_periodicTimer, &QTimer::timeout, this, &UpdateChecker::onPeriodicTimer);
}

QString UpdateChecker::currentVersion() {
#ifdef PROTECTEYE_VERSION
    return QStringLiteral(PROTECTEYE_VERSION);
#else
    return QStringLiteral("1.0.3");
#endif
}

int UpdateChecker::compareVersions(const QString& v1, const QString& v2) {
    QString s1 = v1.trimmed();
    if (s1.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        s1.remove(0, 1);
    }
    QString s2 = v2.trimmed();
    if (s2.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        s2.remove(0, 1);
    }

    // Ignore suffixes like -beta, -rc
    const QStringList p1 = s1.section(QLatin1Char('-'), 0, 0).split(QLatin1Char('.'));
    const QStringList p2 = s2.section(QLatin1Char('-'), 0, 0).split(QLatin1Char('.'));

    const int maxLen = std::max(p1.size(), p2.size());
    for (int i = 0; i < maxLen; ++i) {
        int n1 = (i < p1.size()) ? p1[i].toInt() : 0;
        int n2 = (i < p2.size()) ? p2[i].toInt() : 0;
        if (n1 > n2) return 1;
        if (n1 < n2) return -1;
    }
    return 0;
}

void UpdateChecker::startBackgroundChecks() {
    // Non-blocking quick check shortly after startup (2.5 seconds)
    QTimer::singleShot(2500, this, [this]() {
        checkForUpdates(false, /* isStartup = */ true);
    });

    // Periodic check every 12 hours while running
    m_periodicTimer->start(12 * 3600 * 1000);
}

void UpdateChecker::onPeriodicTimer() {
    checkForUpdates(false, /* isStartup = */ false);
}

void UpdateChecker::checkForUpdates(bool manual, bool isStartup) {
    if (m_isChecking) {
        return;
    }

    if (!manual && !isStartup) {
        if (!Settings::instance().checkUpdatesEnabled()) {
            return;
        }
        const qint64 lastCheck = Settings::instance().lastUpdateCheckTime();
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        // Skip periodic automatic check if checked within the past 11 hours
        if (lastCheck > 0 && (now - lastCheck) < (11 * 3600)) {
            return;
        }
    } else if (isStartup) {
        if (!Settings::instance().checkUpdatesEnabled()) {
            return;
        }
    }

    m_isChecking = true;
    emit checkStarted();
    queryGithubApi(manual, isStartup);
}

void UpdateChecker::queryGithubApi(bool manual, bool isStartup) {
    const QUrl url(QStringLiteral("https://api.github.com/repos/alierenaltindag/protect-eye/releases/latest"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ProtectEye-App/%1").arg(currentVersion()));
    request.setRawHeader("Accept", "application/vnd.github.v3+json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(12000); // 12 seconds timeout

    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError>& errors) {
        qWarning() << "UpdateChecker: SSL warnings on GitHub API request:" << errors;
        reply->ignoreSslErrors();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, manual, isStartup]() {
        reply->deleteLater();

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray data = reply->readAll();

        if (reply->error() != QNetworkReply::NoError || statusCode >= 400 || data.isEmpty()) {
            QString err = reply->errorString();
            qWarning().noquote() << QStringLiteral("UpdateChecker: Primary GitHub API check failed (status %1: %2). Trying CDN fallback...")
                                    .arg(statusCode)
                                    .arg(err);
            queryFallbackVersion(manual, isStartup, err);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (doc.isNull() || !doc.isObject()) {
            qWarning() << "UpdateChecker: Invalid JSON response from server. Trying CDN fallback...";
            queryFallbackVersion(manual, isStartup, QStringLiteral("Invalid JSON response"));
            return;
        }

        const QJsonObject obj = doc.object();
        const QString tagName = obj.value(QStringLiteral("tag_name")).toString();
        const QString htmlUrl = obj.value(QStringLiteral("html_url")).toString();
        const QString body = obj.value(QStringLiteral("body")).toString();

        if (tagName.isEmpty()) {
            qWarning() << "UpdateChecker: No release tag found in JSON. Trying CDN fallback...";
            queryFallbackVersion(manual, isStartup, QStringLiteral("No release tag found"));
            return;
        }

        m_isChecking = false;
        processVersionResult(tagName, htmlUrl, body, manual);
    });
}

void UpdateChecker::queryFallbackVersion(bool manual, bool isStartup, const QString& primaryError) {
    Q_UNUSED(isStartup);
    const QUrl fallbackUrl(QStringLiteral("https://raw.githubusercontent.com/alierenaltindag/protect-eye/main/VERSION"));
    QNetworkRequest request(fallbackUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ProtectEye-App/%1").arg(currentVersion()));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(10000);

    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError>& errors) {
        qWarning() << "UpdateChecker: SSL warnings on fallback request:" << errors;
        reply->ignoreSslErrors();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, manual, primaryError]() {
        reply->deleteLater();
        m_isChecking = false;

        if (reply->error() != QNetworkReply::NoError) {
            QString finalError = !primaryError.isEmpty() ? primaryError : reply->errorString();
            qWarning().noquote() << "UpdateChecker: Fallback check failed as well:" << reply->errorString();
            emit checkFinished(false, QString(), finalError);
            return;
        }

        QString verStr = QString::fromUtf8(reply->readAll()).trimmed();
        static const QRegularExpression semVerRegex(QStringLiteral(R"(^[0-9]+\.[0-9]+\.[0-9]+)"));
        if (!semVerRegex.match(verStr).hasMatch()) {
            qWarning().noquote() << "UpdateChecker: Invalid version string received from fallback:" << verStr;
            emit checkFinished(false, QString(), primaryError.isEmpty() ? QStringLiteral("Invalid fallback version") : primaryError);
            return;
        }

        qInfo().noquote() << "UpdateChecker: Successfully checked version via CDN fallback:" << verStr;
        processVersionResult(
            verStr,
            QStringLiteral("https://github.com/alierenaltindag/protect-eye/releases/latest"),
            QString(),
            manual
        );
    });
}

void UpdateChecker::processVersionResult(const QString& version, const QString& releaseUrl, const QString& releaseNotes, bool manual) {
    Q_UNUSED(manual);
    // Record successful update check timestamp
    Settings::instance().setLastUpdateCheckTime(QDateTime::currentSecsSinceEpoch());
    Settings::instance().save();

    QString cleanTag = version.trimmed();
    if (cleanTag.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        cleanTag.remove(0, 1);
    }

    m_latestVersion = cleanTag;
    m_releaseUrl = releaseUrl.isEmpty() ? QStringLiteral("https://github.com/alierenaltindag/protect-eye/releases/latest") : releaseUrl;

    if (compareVersions(cleanTag, currentVersion()) > 0) {
        emit updateAvailable(cleanTag, m_releaseUrl, releaseNotes);
        emit checkFinished(true, cleanTag, QString());
    } else {
        emit checkFinished(false, cleanTag, QString());
    }
}

int UpdateChecker::runCliUpdate(int argc, char* argv[]) {
    auto& loc = Localization::instance();
    qInfo().noquote() << loc.get(
        QStringLiteral("update_running"),
        QStringLiteral("Checking and updating ProtectEye to the latest version...")
    );

#ifdef Q_OS_WIN
    qInfo().noquote() << loc.get(
        QStringLiteral("update_win_hint"),
        QStringLiteral("Downloading latest Windows installer and launching update...")
    );

    QString psCommand = QStringLiteral(
        "powershell -NoProfile -ExecutionPolicy Bypass -Command \""
        "$ProgressPreference = 'SilentlyContinue'; "
        "try { "
        "   $rel = Invoke-RestMethod -Uri 'https://api.github.com/repos/alierenaltindag/protect-eye/releases/latest'; "
        "   $latestVer = $rel.tag_name.TrimStart('v').Trim(); "
        "   $currentVer = '%1'; "
        "   if ([System.Version]$latestVer -le [System.Version]$currentVer) { "
        "       Write-Host ('[✓] ProtectEye is already up to date (v' + $currentVer + ').'); exit 0; "
        "   } "
        "   Write-Host ('==> New version available: v' + $latestVer + ' (Current: v' + $currentVer + ')'); "
        "   $asset = $rel.assets | Where-Object { $_.name -like '*_Setup.exe' } | Select-Object -First 1; "
        "   if (-not $asset) { Write-Host '[!] No Windows setup asset found in release.' -ForegroundColor Red; exit 1; } "
        "   $tempPath = [System.IO.Path]::Combine($env:TEMP, $asset.name); "
        "   Write-Host ('==> Downloading ' + $asset.name + '...'); "
        "   Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $tempPath; "
        "   Write-Host '==> Installing update...'; "
        "   Stop-Process -Name 'protecteye' -Force -ErrorAction SilentlyContinue; "
        "   Start-Process -FilePath $tempPath -ArgumentList '/SILENT' -Wait; "
        "   Write-Host ('[✓] ProtectEye successfully updated to v' + $latestVer + '!'); "
        "   $installedExe = Join-Path $env:LOCALAPPDATA 'Programs\\ProtectEye\\protecteye.exe'; "
        "   if (Test-Path $installedExe) { Start-Process -FilePath $installedExe; } "
        "} catch { "
        "   Write-Host ('[!] Update failed: ' + $_.Exception.Message) -ForegroundColor Red; exit 1; "
        "}\""
    ).arg(currentVersion());

    return std::system(qPrintable(psCommand));
#else
    QStringList candidates = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/update.sh"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../update.sh"),
        QStringLiteral("/usr/local/share/protecteye/update.sh"),
        QStringLiteral("/usr/share/protecteye/update.sh")
    };
    QString foundScript;
    for (const auto& path : candidates) {
        if (QFile::exists(path)) {
            foundScript = path;
            break;
        }
    }

    QString extraArgs;
    for (int i = 2; i < argc; ++i) {
        extraArgs += QStringLiteral(" ") + QString::fromUtf8(argv[i]);
    }

    if (!foundScript.isEmpty()) {
        return std::system(qPrintable(QStringLiteral("bash \"%1\"%2").arg(foundScript, extraArgs)));
    }

    // Fallback to online script
    return std::system(qPrintable(
        QStringLiteral("bash -c \"$(curl -fsSL https://raw.githubusercontent.com/alierenaltindag/protect-eye/main/update.sh)\" protecteye-update%1").arg(extraArgs)
    ));
#endif
}
