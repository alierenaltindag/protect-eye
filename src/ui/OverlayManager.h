#pragma once

#include <QObject>
#include <QList>
#include <QStringList>
#include "OverlayWindow.h"

class OverlayManager : public QObject {
    Q_OBJECT

public:
    explicit OverlayManager(QObject* parent = nullptr);
    ~OverlayManager() override;

    void showBreak(bool isLong, int durationSec);
    void updateCountdown(int remainingSec, int totalSec);
    void closeBreak();

signals:
    void skipRequested();
    void snoozeRequested();

private slots:
    void onScreenAdded(QScreen* screen);
    void onScreenRemoved(QScreen* screen);

private:
    QList<OverlayWindow*> m_overlays;
    bool m_isBreakActive{false};
    bool m_currentIsLong{false};
    int m_currentDuration{15};
    int m_currentRemainingSec{15};
    ExerciseGuide m_currentExercise;
    int m_shortCycleIndex{0};
    int m_longCycleIndex{0};
};

