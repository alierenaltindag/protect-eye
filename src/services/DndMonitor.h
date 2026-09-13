#pragma once

#include <QObject>
#include <QString>

class DndMonitor : public QObject {
    Q_OBJECT

public:
    explicit DndMonitor(QObject* parent = nullptr);
    ~DndMonitor() override = default;

    // Returns true if Do Not Disturb mode is currently active
    virtual bool isDndActive() const;

private:
#ifdef Q_OS_WIN
    bool checkWindowsDnd() const;
#else
    bool checkGnomeDnd() const;
    bool checkKdeDnd() const;
    bool checkXfceDnd() const;
    bool checkCinnamonDnd() const;
    bool checkMateDnd() const;
    bool checkSwayNcDnd() const;
    bool checkDunstDnd() const;
    bool checkFreedesktopInhibited() const;
    bool checkX11Fullscreen() const;
    QString m_desktopEnvironment;
    mutable int m_cachedFallbackMethod{-1};
#endif
};
