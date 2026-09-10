#ifndef MUSICLINKMANAGER_H
#define MUSICLINKMANAGER_H

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVector>

#include "../pomodoro/pomodorotimer.h"
#include "musiclinkresolver.h"
#include "musicplayer.h"

// Manages the user's saved Music Links (labeled YouTube URLs) for the
// Pomodoro view: adding/removing them, tracking which one is selected, and
// the playback volume (selection and volume persist across restarts), plus
// playing the selected Music Link in sync with the Pomodoro timer.
//
// Playback is modeled uniformly as an ordered list of "entries": a
// single-video Music Link resolves to one entry, a playlist Music Link to
// several. This lets single-video looping (ticket 3) and playlist
// advance/loop-back/skip-broken-entry (ticket 4) share one code path - a
// single video is just a playlist of one, so it "loops back" to itself.
//
// Resolving a Music Link's audio and actually playing it are both behind
// injectable seams (MusicLinkResolver, MusicPlayer) so this orchestration can
// be tested with fakes, without a real yt-dlp process or real audio.
class MusicLinkManager : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int selectedLinkId READ selectedLinkId WRITE selectMusicLink NOTIFY selectedLinkIdChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(QString currentEntryUrl READ currentEntryUrl NOTIFY currentEntryChanged)
    Q_PROPERTY(QString playbackError READ playbackError NOTIFY playbackErrorChanged)

public:
    explicit MusicLinkManager(PomodoroTimer *pomodoro, QObject *parent = nullptr);
    // Test/DI seam: caller retains ownership of resolver and player.
    MusicLinkManager(PomodoroTimer *pomodoro, MusicLinkResolver *resolver, MusicPlayer *player,
                      QObject *parent = nullptr);
    ~MusicLinkManager() override;

    enum MusicLinkRoles {
        IdRole = Qt::UserRole + 1,
        LabelRole,
        UrlRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addMusicLink(const QString &label, const QString &url);
    Q_INVOKABLE void removeMusicLink(int index);
    Q_INVOKABLE int count() const;

    int selectedLinkId() const;
    void selectMusicLink(int id);

    qreal volume() const;
    void setVolume(qreal volume);

    bool isPlaying() const;
    QString currentEntryUrl() const;
    QString playbackError() const;

    // Mirrors PomodoroTimer's pause()/resume(), which have no distinct
    // pause/resume signal of their own to react to - called directly from
    // QML alongside pomodoroTimer.pause()/resume().
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();

signals:
    void musicLinkAdded();
    void musicLinkRemoved();
    void selectedLinkIdChanged();
    void volumeChanged();
    void playingChanged();
    void currentEntryChanged();
    void playbackErrorChanged();

private slots:
    void onPomodoroStateChanged(PomodoroTimer::State state);
    void onEntryFinished();
    void onEntryPlaybackError();

private:
    struct MusicLinkEntry {
        int id;
        QString label;
        QString url;
    };

    void loadFromDatabase();
    void persistSettings();

    void startPlayback();
    void stopPlayback();
    // Tries each entry starting at startIndex, wrapping around at most once
    // through the whole playlist; stops at the first that resolves and plays,
    // or falls silent if every entry fails. Returns whether playback started.
    bool tryPlayFromIndex(int startIndex);
    void skipToNextEntry();

    void setPlaying(bool playing);
    void setCurrentEntryUrl(const QString &url);
    void setPlaybackError(const QString &message);

    PomodoroTimer *m_pomodoro;
    QVector<MusicLinkEntry> m_links;
    int m_selectedLinkId;
    qreal m_volume;

    MusicLinkResolver *m_resolver;
    MusicPlayer *m_player;
    bool m_ownsResolver;
    bool m_ownsPlayer;

    QVector<QString> m_entries;
    int m_currentEntryIndex;
    bool m_playing;
    QString m_currentEntryUrl;
    QString m_playbackError;
    // Counts entries skipped via a real playback failure (as opposed to a
    // resolve failure) since the last successful play; caps the otherwise
    // unbounded onEntryPlaybackError -> skip -> resolve -> play -> error cycle
    // that a resolve-fine-but-unplayable stream would trigger.
    int m_consecutivePlaybackErrors;
};

#endif // MUSICLINKMANAGER_H
