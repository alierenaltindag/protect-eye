#include "OverlayManager.h"
#include "core/Localization.h"
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>

OverlayManager::OverlayManager(QObject* parent)
    : QObject(parent) {
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &OverlayManager::onScreenAdded);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &OverlayManager::onScreenRemoved);
}

OverlayManager::~OverlayManager() {
    m_isBreakActive = false;
    for (auto* overlay : m_overlays) {
        if (overlay) {
            overlay->deleteLater();
        }
    }
    m_overlays.clear();
}

void OverlayManager::showBreak(bool isLong, int durationSec) {
    m_isBreakActive = true;
    m_currentIsLong = isLong;
    m_currentDuration = durationSec;
    m_currentRemainingSec = durationSec;

    if (isLong) {
        m_currentExercise = Localization::instance().getLongBreakExercise(m_longCycleIndex++);
    } else {
        m_currentExercise = Localization::instance().getShortBreakExercise(m_shortCycleIndex++);
    }

    // Recreate or refresh overlays for each screen
    closeBreak();
    m_isBreakActive = true;

    const auto screens = QGuiApplication::screens();
    for (QScreen* screen : screens) {
        auto* overlay = new OverlayWindow(screen);
        connect(overlay, &OverlayWindow::skipRequested, this, &OverlayManager::skipRequested);
        connect(overlay, &OverlayWindow::snoozeRequested, this, &OverlayManager::snoozeRequested);
        
        overlay->prepareBreak(isLong, durationSec, m_currentExercise);
        overlay->showFullScreen();
        overlay->raise();
        overlay->activateWindow();
        m_overlays.append(overlay);
    }
}

void OverlayManager::updateCountdown(int remainingSec, int totalSec) {
    if (!m_isBreakActive) return;
    m_currentRemainingSec = remainingSec;

    for (auto* overlay : m_overlays) {
        if (overlay) {
            overlay->updateCountdown(remainingSec, totalSec);
        }
    }
}

void OverlayManager::closeBreak() {
    m_isBreakActive = false;
    for (auto* overlay : m_overlays) {
        if (overlay) {
            // Disable interactions immediately so rapid clicks are ignored
            overlay->setEnabled(false);
            // Defer hide & deleteLater to the next event loop iteration so any
            // active mouse/keyboard event dispatch unwinds cleanly on Wayland/X11
            QTimer::singleShot(0, overlay, [overlay]() {
                overlay->hide();
                overlay->deleteLater();
            });
        }
    }
    m_overlays.clear();
}

void OverlayManager::onScreenAdded(QScreen* screen) {
    if (m_isBreakActive && screen) {
        auto* overlay = new OverlayWindow(screen);
        connect(overlay, &OverlayWindow::skipRequested, this, &OverlayManager::skipRequested);
        connect(overlay, &OverlayWindow::snoozeRequested, this, &OverlayManager::snoozeRequested);
        overlay->prepareBreak(m_currentIsLong, m_currentDuration, m_currentExercise);
        overlay->updateCountdown(m_currentRemainingSec, m_currentDuration);
        overlay->showFullScreen();
        overlay->raise();
        overlay->activateWindow();
        m_overlays.append(overlay);
    }
}

void OverlayManager::onScreenRemoved(QScreen* screen) {
    for (int i = m_overlays.size() - 1; i >= 0; --i) {
        if (m_overlays[i]->screen() == screen) {
            auto* overlay = m_overlays.takeAt(i);
            overlay->setEnabled(false);
            QTimer::singleShot(0, overlay, [overlay]() {
                overlay->hide();
                overlay->deleteLater();
            });
        }
    }
}
