#include "qtmediamusicplayer.h"

#include <QAudio>
#include <QAudioOutput>
#include <QDebug>
#include <QUrl>

QtMediaMusicPlayer::QtMediaMusicPlayer(QObject *parent)
    : MusicPlayer(parent),
    m_player(new QMediaPlayer(this)),
    m_audioOutput(new QAudioOutput(this))
{
    m_player->setAudioOutput(m_audioOutput);

    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &QtMediaMusicPlayer::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &QtMediaMusicPlayer::onErrorOccurred);
}

void QtMediaMusicPlayer::play(const QString &streamUrl)
{
    m_player->setSource(QUrl(streamUrl));
    m_player->play();
}

void QtMediaMusicPlayer::pause()
{
    m_player->pause();
}

void QtMediaMusicPlayer::resume()
{
    m_player->play();
}

void QtMediaMusicPlayer::stop()
{
    m_player->stop();
    m_player->setSource(QUrl());
}

void QtMediaMusicPlayer::setVolume(qreal volume)
{
    // The incoming value comes straight off a UI slider, where the user
    // expects the midpoint to sound like "half as loud". QAudioOutput takes a
    // linear amplitude, in which 0.5 is only about -6 dB - perceptually closer
    // to 3/4 volume, making the top half of the slider feel inert.
    m_audioOutput->setVolume(
        QAudio::convertVolume(volume, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale));
}

void QtMediaMusicPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia)
        emit finished();
}

void QtMediaMusicPlayer::onErrorOccurred(QMediaPlayer::Error error, const QString &errorString)
{
    if (error == QMediaPlayer::NoError)
        return;

    qDebug() << "QtMediaMusicPlayer playback error:" << errorString;
    emit errorOccurred();
}
