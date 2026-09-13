#ifndef MUSICLINKMANAGER_H
#define MUSICLINKMANAGER_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QElapsedTimer>
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
//
// Resolution is asynchronous, so this is a small state machine rather than a
// loop: a request goes out, and the answer arrives in onEntriesReady() /
// onStreamUrlReady() some time later, possibly after the user has already
// stopped the session or picked a different Music Link. Every reset bumps a
// generation counter and cancels the resolver, so answers to superseded
// questions are dropped instead of acted on.
class MusicLinkManager : public QAbstractListModel
{
    Q_OBJECT

    // A real property, not just the Q_INVOKABLE below: PomodoroView binds this
    // to choose between the saved-links list and the empty-state message, and a
    // bare method would resolve to the function object there, leaving both
    // hidden no matter how many links exist.
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int selectedLinkId READ selectedLinkId WRITE selectMusicLink NOTIFY selectedLinkIdChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(QString currentEntryUrl READ currentEntryUrl NOTIFY currentEntryChanged)
    Q_PROPERTY(QString playbackError READ playbackError NOTIFY playbackErrorChanged)

public:
    // Timing knobs for playback recovery. The defaults are what the app runs
    // with; tests override them so they never have to wait out a real delay.
    struct PlaybackTuning {
        // How long to wait before re-attempting a stream resolve that just
        // failed. A retry fired immediately just re-asks during the same
        // network blip it exists to ride out.
        int retryDelayMs = 750;
        // How long an entry must have been playing for a later playback error
        // to count as "it worked, then broke" rather than "it never played".
        // Only the latter accumulates toward the give-up cap, so an hour of
        // occasional recovered hiccups can't silently exhaust it.
        int healthyPlaybackMs = 30000;
        // How long a prefetched stream URL stays worth using. yt-dlp hands back
        // a signed, expiring link, so past this it is re-resolved rather than
        // played and found dead.
        int prefetchFreshnessMs = 300000;
        // How long a selection has to settle before it is worth resolving.
        // Clicking down a list of Music Links would otherwise spawn, and then
        // kill, a yt-dlp process per click.
        int prefetchDebounceMs = 400;
    };

    // resolver and player are required collaborators and the caller keeps
    // ownership of both (main.cpp parents them to the application). Composing
    // them here instead would drag QProcess and QMediaPlayer into every target
    // that links this class, tests included.
    MusicLinkManager(PomodoroTimer *pomodoro, MusicLinkResolver *resolver, MusicPlayer *player,
                      QObject *parent = nullptr);

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

    void setPlaybackTuning(const PlaybackTuning &tuning);

signals:
    void musicLinkAdded();
    void musicLinkRemoved();
    void countChanged();
    void selectedLinkIdChanged();
    void volumeChanged();
    void playingChanged();
    void currentEntryChanged();
    void playbackErrorChanged();

private slots:
    void onPomodoroStateChanged(PomodoroTimer::State state);
    void onPomodoroPaused();
    void onPomodoroResumed();
    void onEntriesReady(const QString &musicLinkUrl, const QVector<QString> &entries);
    void onStreamUrlReady(const QString &entryUrl, const QString &streamUrl);
    void onEntryFinished();
    void onEntryPlaybackError();

private:
    struct SavedMusicLink {
        int id;
        QString label;
        QString url;
    };

    struct PrefetchedLink {
        int linkId = -1;
        QString musicLinkUrl;
        QVector<QString> entries;
        QString firstStreamUrl;
        // Wall clock, not QElapsedTimer: the monotonic clock stops while the
        // machine is suspended, so a link picked before closing the lid would
        // still look fresh the next morning - exactly when its signed stream
        // URL has expired.
        QDateTime resolvedAt;
    };

    void loadFromDatabase();
    void persistSettings();

    QString selectedLinkUrl() const;

    void startPlayback();
    void stopPlayback();
    // Drops all playback state and detaches from resolver and player. Two
    // deliberate exceptions: playbackError, because the caller decides whether
    // falling out of playback is worth telling the user about; and the prefetch
    // cache, because Start calls this immediately before going looking for it.
    void clearPlaybackState();

    // One pass over the playlist starting at startIndex, trying each entry at
    // most once (plus a delayed retry each) and stopping at the first that
    // resolves. Driven by onStreamUrlReady(), not a loop.
    void beginResolveSweep(int startIndex);
    void requestEntryAt(int index);
    void retryPendingEntry();
    void advanceSweepOrGiveUp();
    void skipToNextEntry();
    void playResolvedEntry(int index, const QString &streamUrl);

    // Resolve a Music Link ahead of Start, so pressing Start doesn't wait on
    // yt-dlp. Speculative throughout: a prefetch that fails is simply dropped,
    // never surfaced, and Start falls back to resolving for real.
    void prefetchSelectedLink();
    void beginPrefetchRequest();
    bool prefetchIsUsable() const;
    // True while playback is waiting on a resolver answer of its own, which
    // speculation must never cut across.
    bool playbackResolveInFlight() const;
    // Starts playback from a prefetched result, if one fits the current
    // selection. Returns whether it did.
    bool startFromPrefetch();
    void onPrefetchEntriesReady(const QString &musicLinkUrl, const QVector<QString> &entries);
    void onPrefetchStreamUrlReady(const QString &entryUrl, const QString &streamUrl);
    void discardPrefetch();

    void setActive(bool active);
    void setPaused(bool paused);
    void notifyPlayingChanged();
    void setCurrentEntryUrl(const QString &url);
    void setPlaybackError(const QString &message);

    PomodoroTimer *m_pomodoro;
    QVector<SavedMusicLink> m_links;
    int m_selectedLinkId;
    qreal m_volume;

    MusicLinkResolver *m_resolver;
    MusicPlayer *m_player;
    PlaybackTuning m_tuning;

    // True from the moment a session starts working until it goes Idle,
    // spanning breaks - i.e. "this session still owns the music".
    bool m_sessionActive;

    bool m_prefetchInFlight;
    // Strands a debounce still counting down, or an answer to speculation
    // nobody is waiting for any more.
    int m_prefetchGeneration;
    PrefetchedLink m_prefetch;

    QVector<QString> m_entries;
    int m_currentEntryIndex;

    // Bumped by every reset and every new entry request; a resolver answer or
    // queued retry carrying a stale generation is one nobody is waiting for.
    int m_resolveGeneration;
    QString m_pendingMusicLinkUrl;
    int m_pendingEntryIndex;
    bool m_pendingRetryUsed;
    int m_sweepStart;
    int m_sweepTried;

    // m_active is "this session has music"; m_paused is "and it's suspended".
    // The QML-facing playing property is the conjunction, so a paused session
    // doesn't claim to be playing.
    bool m_active;
    bool m_paused;
    bool m_reportedPlaying;
    // An entry that ended or failed while paused: act on it at resume, so a
    // paused session never starts making noise on its own.
    bool m_advanceWhenResumed;

    QString m_currentEntryUrl;
    QString m_playbackError;
    // Counts entries that failed during actual playback (as opposed to a
    // resolve failure) without any of them having played healthily in between;
    // caps the otherwise unbounded error -> skip -> resolve -> play -> error
    // cycle that a resolve-fine-but-unplayable stream would trigger.
    int m_consecutivePlaybackErrors;
    QElapsedTimer m_currentEntryPlayingSince;
};

#endif // MUSICLINKMANAGER_H
