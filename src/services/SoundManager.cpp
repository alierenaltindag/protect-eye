#include "SoundManager.h"
#include "core/Settings.h"
#include <QApplication>
#include <QUrl>

SoundManager::SoundManager(QObject* parent)
    : QObject(parent) {
    m_startSound.setSource(QUrl(QStringLiteral("qrc:/chime_start.wav")));
    m_startSound.setVolume(0.30f);

    m_endSound.setSource(QUrl(QStringLiteral("qrc:/chime_end.wav")));
    m_endSound.setVolume(0.30f);

}

void SoundManager::playBreakStart() {
    if (!Settings::instance().soundEnabled()) {
        return;
    }
    m_startSound.play();
}

void SoundManager::playBreakEnd() {
    if (!Settings::instance().soundEnabled()) {
        return;
    }
    m_endSound.play();
}
