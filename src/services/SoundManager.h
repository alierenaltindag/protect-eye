#pragma once

#include <QObject>
#include <QSoundEffect>
#include <memory>

class SoundManager : public QObject {
    Q_OBJECT

public:
    explicit SoundManager(QObject* parent = nullptr);
    ~SoundManager() override = default;

    void playBreakStart();
    void playBreakEnd();

private:
    QSoundEffect m_startSound;
    QSoundEffect m_endSound;
};
