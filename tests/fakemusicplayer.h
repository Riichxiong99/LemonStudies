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
    // Fails from inside play(), the way a player can when the source is
    // rejected outright. Re-enters MusicLinkManager while it is still in the
    // middle of starting an entry, which is what the ordering of state commit
    // versus player call has to survive.
    bool failSynchronouslyOnPlay = false;
    int pauseCallCount = 0;
    int resumeCallCount = 0;
    int stopCallCount = 0;
    qreal lastVolume = -1;

    void play(const QString &streamUrl) override
    {
        playedStreamUrls.append(streamUrl);
        if (failSynchronouslyOnPlay)
            emit errorOccurred();
    }
    void pause() override { pauseCallCount++; }
    void resume() override { resumeCallCount++; }
    void stop() override { stopCallCount++; }
    void setVolume(qreal volume) override { lastVolume = volume; }

    void simulateFinished() { emit finished(); }
    void simulateError() { emit errorOccurred(); }
};

#endif // FAKEMUSICPLAYER_H
