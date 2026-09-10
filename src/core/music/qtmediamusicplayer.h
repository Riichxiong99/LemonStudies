#ifndef QTMEDIAMUSICPLAYER_H
#define QTMEDIAMUSICPLAYER_H

#include "musicplayer.h"

#include <QMediaPlayer>

class QAudioOutput;

// Production MusicPlayer: wraps QMediaPlayer/QAudioOutput (see ADR 0001 —
// in-process playback via QtMultimedia, not an embedded browser).
class QtMediaMusicPlayer : public MusicPlayer
{
    Q_OBJECT

public:
    explicit QtMediaMusicPlayer(QObject *parent = nullptr);

    void play(const QString &streamUrl) override;
    void pause() override;
    void resume() override;
    void stop() override;
    void setVolume(qreal volume) override;

private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onErrorOccurred(QMediaPlayer::Error error, const QString &errorString);

private:
    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
};

#endif // QTMEDIAMUSICPLAYER_H
