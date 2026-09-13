#include "AutostartHelper.h"
#include <QCoreApplication>
#include <QDir>
#include <QSettings>

#ifdef Q_OS_WIN
bool AutostartHelper::isAutostartEnabled() {
    QSettings runSettings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
    return runSettings.contains(QStringLiteral("ProtectEye"));
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
    return configDir + "/autostart/protecteye.desktop";
}

bool AutostartHelper::isAutostartEnabled() {
    QString userDesktopPath = getAutostartFilePath();
    if (QFile::exists(userDesktopPath)) {
        QFile file(userDesktopPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.compare(QStringLiteral("Hidden=true"), Qt::CaseInsensitive) == 0 ||
                    line.compare(QStringLiteral("X-GNOME-Autostart-enabled=false"), Qt::CaseInsensitive) == 0) {
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
            out << "Exec=\"" << QCoreApplication::applicationFilePath() << "\" --autostart\n";
            out << "Hidden=true\n";
            out << "X-GNOME-Autostart-enabled=false\n";
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
        out << "Exec=\"" << QCoreApplication::applicationFilePath() << "\" --autostart\n";
        out << "Icon=protecteye\n";
        out << "Terminal=false\n";
        out << "Categories=Utility;Qt;\n";
        out << "StartupNotify=false\n";
        out << "X-GNOME-Autostart-enabled=true\n";
        out << "X-KDE-autostart-after=panel\n";
        out << "X-MATE-Autostart-enabled=true\n";
        out << "X-XFCE-Autostart-enabled=true\n";
        file.close();
    }
}
#endif
