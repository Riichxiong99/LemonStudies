#ifndef FAKEMUSICPLAYER_H
#define FAKEMUSICPLAYER_H

#include "../src/core/music/musicplayer.h"

#include <QStringList>

// Test double for MusicPlayer. Records calls instead of touching real audio;
// tests drive "entry finished" / "entry failed" explicitly via
// simulateFinished()/simulateError() rather than waiting on real playback.
class FakeMusicPlayer : public MusicPlayer
{
    Q_OBJECT

public:
    explicit FakeMusicPlayer(QObject *parent = nullptr) : MusicPlayer(parent) {}

    QStringList playedStreamUrls;
    int pauseCallCount = 0;
    int resumeCallCount = 0;
    int stopCallCount = 0;
    qreal lastVolume = -1;

    void play(const QString &streamUrl) override { playedStreamUrls.append(streamUrl); }
    void pause() override { pauseCallCount++; }
    void resume() override { resumeCallCount++; }
    void stop() override { stopCallCount++; }
    void setVolume(qreal volume) override { lastVolume = volume; }

    void simulateFinished() { emit finished(); }
    void simulateError() { emit errorOccurred(); }
};

#endif // FAKEMUSICPLAYER_H
