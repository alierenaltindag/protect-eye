#include "UpdateChecker.h"
#include "core/Settings.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>
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
    // Non-blocking delayed check on startup (15 seconds)
    QTimer::singleShot(15000, this, [this]() {
        checkForUpdates(false);
    });

    // Periodic check every 24 hours
    m_periodicTimer->start(24 * 3600 * 1000);
}

void UpdateChecker::onPeriodicTimer() {
    checkForUpdates(false);
}

void UpdateChecker::checkForUpdates(bool manual) {
    if (m_isChecking) {
        return;
    }

    if (!manual) {
        if (!Settings::instance().checkUpdatesEnabled()) {
            return;
        }
        const qint64 lastCheck = Settings::instance().lastUpdateCheckTime();
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        // Skip automatic checks if checked within the past 23 hours
        if (lastCheck > 0 && (now - lastCheck) < (23 * 3600)) {
            return;
        }
    }

    m_isChecking = true;
    emit checkStarted();

    const QUrl url(QStringLiteral("https://api.github.com/repos/alierenaltindag/protect-eye/releases/latest"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ProtectEye-App/%1").arg(currentVersion()));
    request.setRawHeader("Accept", "application/vnd.github.v3+json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(10000); // 10 seconds timeout

    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, manual]() {
        reply->deleteLater();
        m_isChecking = false;

        if (reply->error() != QNetworkReply::NoError) {
            emit checkFinished(false, QString(), reply->errorString());
            return;
        }

        // Record successful update check timestamp
        Settings::instance().setLastUpdateCheckTime(QDateTime::currentSecsSinceEpoch());
        Settings::instance().save();

        const QByteArray data = reply->readAll();
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (doc.isNull() || !doc.isObject()) {
            emit checkFinished(false, QString(), QStringLiteral("Invalid JSON response from server"));
            return;
        }

        const QJsonObject obj = doc.object();
        const QString tagName = obj.value(QStringLiteral("tag_name")).toString();
        const QString htmlUrl = obj.value(QStringLiteral("html_url")).toString();
        const QString body = obj.value(QStringLiteral("body")).toString();

        if (tagName.isEmpty()) {
            emit checkFinished(false, QString(), QStringLiteral("No release tag found"));
            return;
        }

        QString cleanTag = tagName.trimmed();
        if (cleanTag.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
            cleanTag.remove(0, 1);
        }

        m_latestVersion = cleanTag;
        m_releaseUrl = htmlUrl.isEmpty() ? QStringLiteral("https://github.com/alierenaltindag/protect-eye/releases/latest") : htmlUrl;

        if (compareVersions(cleanTag, currentVersion()) > 0) {
            emit updateAvailable(cleanTag, m_releaseUrl, body);
            emit checkFinished(true, cleanTag, QString());
        } else {
            emit checkFinished(false, cleanTag, QString());
        }
    });
}
