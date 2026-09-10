#include "qtmediamusicplayer.h"

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
    m_audioOutput->setVolume(volume);
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
