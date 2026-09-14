#include "AutostartHelper.h"
#include <QCoreApplication>
#include <QDir>
#include <QSettings>

#ifdef Q_OS_WIN
bool AutostartHelper::isAutostartEnabled() {
    QSettings runSettings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
    if (!runSettings.contains(QStringLiteral("ProtectEye"))) {
        return false;
    }
    QString val = runSettings.value(QStringLiteral("ProtectEye")).toString().trimmed();
    return !val.isEmpty();
}

void AutostartHelper::setAutostartEnabled(bool enable) {
    QSettings runSettings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
    if (enable) {
        QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        runSettings.setValue(QStringLiteral("ProtectEye"), QStringLiteral("\"%1\" --autostart").arg(appPath));
    } else {
        runSettings.remove(QStringLiteral("ProtectEye"));
    }
}

#else

#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include "core/Localization.h"
#include "core/Settings.h"

QString AutostartHelper::getAutostartFilePath() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (configDir.isEmpty()) {
        configDir = QDir::homePath() + QStringLiteral("/.config");
    }
    return configDir + QStringLiteral("/autostart/protecteye.desktop");
}

QString AutostartHelper::getExecutablePath() {
    // If running inside an AppImage, applicationFilePath points to a temporary mount directory in /tmp
    // The AppImage runtime sets the APPIMAGE environment variable to the persistent AppImage file on disk.
    QByteArray appImageEnv = qgetenv("APPIMAGE");
    if (!appImageEnv.isEmpty()) {
        QString appImagePath = QString::fromUtf8(appImageEnv);
        if (QFile::exists(appImagePath)) {
            return appImagePath;
        }
    }
    return QCoreApplication::applicationFilePath();
}

bool AutostartHelper::isAutostartEnabled() {
    QString userDesktopPath = getAutostartFilePath();
    if (QFile::exists(userDesktopPath)) {
        QFile file(userDesktopPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.isEmpty() || line.startsWith('#') || line.startsWith('[')) {
                    continue;
                }
                QString key = line.section('=', 0, 0).trimmed();
                QString val = line.section('=', 1).trimmed();

                if (key.compare(QStringLiteral("Hidden"), Qt::CaseInsensitive) == 0 &&
                    val.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0) {
                    return false;
                }
                if (key.endsWith(QStringLiteral("Autostart-enabled"), Qt::CaseInsensitive) &&
                    val.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0) {
                    return false;
                }
                if (key.compare(QStringLiteral("X-KDE-autostart-condition"), Qt::CaseInsensitive) == 0 &&
                    val.contains(QStringLiteral("false"), Qt::CaseInsensitive)) {
                    return false;
                }
            }
            return true;
        }
    }

    // Check system-wide autostart directory (/etc/xdg/autostart/protecteye.desktop)
    if (QFile::exists(QStringLiteral("/etc/xdg/autostart/protecteye.desktop"))) {
        return true;
    }

    // Default to application settings state
    return Settings::instance().autostartEnabled();
}

void AutostartHelper::setAutostartEnabled(bool enable) {
    QString desktopPath = getAutostartFilePath();
    QFileInfo fileInfo(desktopPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString execPath = getExecutablePath();

    if (!enable) {
        // In accordance with XDG Desktop Application Autostart Specification:
        // Mask both user and system-wide autostart (/etc/xdg/autostart/protecteye.desktop)
        // using Hidden=true and desktop-specific disable flags.
        QFile file(desktopPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n";
            out << "Type=Application\n";
            out << "Name=ProtectEye\n";
            out << "Exec=\"" << execPath << "\" --autostart\n";
            out << "Hidden=true\n";
            out << "X-GNOME-Autostart-enabled=false\n";
            out << "X-KDE-autostart-condition=\n";
            out << "X-MATE-Autostart-enabled=false\n";
            out << "X-XFCE-Autostart-enabled=false\n";
            file.close();
        }
        return;
    }

    QFile file(desktopPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "[Desktop Entry]\n";
        out << "Type=Application\n";
        out << "Name=ProtectEye\n";
        out << "GenericName=" << Localization::instance().trayTooltip() << "\n";
        out << "Comment=" << Localization::instance().trayTooltip() << "\n";
        out << "TryExec=" << execPath << "\n";
        out << "Exec=\"" << execPath << "\" --autostart\n";
        out << "Icon=protecteye\n";
        out << "Terminal=false\n";
        out << "Categories=Utility;Qt;\n";
        out << "StartupNotify=false\n";
        out << "X-GNOME-Autostart-enabled=true\n";
        out << "X-GNOME-Autostart-Delay=2\n";
        out << "X-KDE-autostart-after=panel\n";
        out << "X-MATE-Autostart-enabled=true\n";
        out << "X-XFCE-Autostart-enabled=true\n";
        file.close();
    }
}
#endif
