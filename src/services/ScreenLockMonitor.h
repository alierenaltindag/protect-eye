#pragma once

#include <QObject>

#ifdef Q_OS_WIN
#include <QAbstractNativeEventFilter>
class QWidget;
#endif

class ScreenLockMonitor : public QObject {
    Q_OBJECT

public:
    explicit ScreenLockMonitor(QObject* parent = nullptr);
    ~ScreenLockMonitor() override;

    bool isLocked() const { return m_isLocked; }

signals:
    void lockStateChanged(bool isLocked);

private slots:
#ifndef Q_OS_WIN
    void onGnomeActiveChanged(bool active);
    void onFreedesktopActiveChanged(bool active);
    void onLogindSessionLocked();
    void onLogindSessionUnlocked();
#endif

private:
    void setupConnections();
    void queryInitialState();

    bool m_isLocked{false};

#ifndef Q_OS_WIN
    QString m_sessionPath;
#endif

#ifdef Q_OS_WIN
    class WinSessionEventFilter;
    std::unique_ptr<WinSessionEventFilter> m_eventFilter;
    QWidget* m_msgWindow{nullptr};
#endif
};
