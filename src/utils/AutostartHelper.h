#pragma once

#include <QString>

class AutostartHelper {
public:
    static bool isAutostartEnabled();
    static void setAutostartEnabled(bool enable);
#ifndef Q_OS_WIN
    static QString getAutostartFilePath();
#endif
};
