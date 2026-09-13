#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QProgressBar>
#include <QPropertyAnimation>

#include "core/Localization.h"

class EyeExerciseWidget;

class OverlayWindow : public QWidget {
    Q_OBJECT

public:
    explicit OverlayWindow(QScreen* targetScreen, QWidget* parent = nullptr);
    ~OverlayWindow() override;

    void prepareBreak(bool isLong, int totalDurationSec, const ExerciseGuide& guide);
    void updateCountdown(int remainingSec, int totalSec);

signals:
    void skipRequested();
    void snoozeRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void setupUi();
    void applyStyles();

    QScreen* m_screen;
    bool m_isLong{false};
    int m_totalDuration{15};
    int m_remainingSec{15};

    QLabel* m_badgeLabel{nullptr};
    QLabel* m_titleLabel{nullptr};
    QLabel* m_countdownLabel{nullptr};
    QLabel* m_tipLabel{nullptr};
    EyeExerciseWidget* m_exerciseWidget{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QPropertyAnimation* m_progressAnim{nullptr};
    QPushButton* m_skipBtn{nullptr};
    QPushButton* m_snoozeBtn{nullptr};
};
