#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <QObject>
#include <QString>

// Plays a single resolved audio stream URL. A QObject (rather than a plain
// interface) so it can signal MusicLinkManager when the current entry ends or
// fails, the same way the real player (QMediaPlayer) does.
class MusicPlayer : public QObject
{
    Q_OBJECT

public:
    explicit MusicPlayer(QObject *parent = nullptr) : QObject(parent) {}

    virtual void play(const QString &streamUrl) = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void stop() = 0;
    virtual void setVolume(qreal volume) = 0;

signals:
    // The current entry finished playing on its own (reached the end).
    void finished();
    // The current entry failed during playback (as opposed to a resolve failure).
    void errorOccurred();
};

#endif // MUSICPLAYER_H
