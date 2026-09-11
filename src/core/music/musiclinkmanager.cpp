#include "musiclinkmanager.h"
#include "../database/databasemanager.h"

#include <QSqlQuery>
#include <QTimer>
#include <algorithm>

MusicLinkManager::MusicLinkManager(PomodoroTimer *pomodoro, MusicLinkResolver *resolver, MusicPlayer *player,
                                    QObject *parent)
    : QAbstractListModel(parent),
    m_pomodoro(pomodoro),
    m_selectedLinkId(-1),
    m_volume(0.5),
    m_resolver(resolver),
    m_player(player),
    m_sessionActive(false),
    m_prefetchInFlight(false),
    m_prefetchGeneration(0),
    m_currentEntryIndex(-1),
    m_resolveGeneration(0),
    m_pendingEntryIndex(-1),
    m_pendingRetryUsed(false),
    m_sweepStart(0),
    m_sweepTried(0),
    m_active(false),
    m_paused(false),
    m_reportedPlaying(false),
    m_advanceWhenResumed(false),
    m_consecutivePlaybackErrors(0)
{
    DatabaseManager::instance().createMusicTables();
    loadFromDatabase();

    if (m_resolver) {
        connect(m_resolver, &MusicLinkResolver::entriesReady, this, &MusicLinkManager::onEntriesReady);
        connect(m_resolver, &MusicLinkResolver::streamUrlReady, this, &MusicLinkManager::onStreamUrlReady);
    }

    if (m_player) {
        m_player->setVolume(m_volume);
        connect(m_player, &MusicPlayer::finished, this, &MusicLinkManager::onEntryFinished);
        connect(m_player, &MusicPlayer::errorOccurred, this, &MusicLinkManager::onEntryPlaybackError);
    }

    if (m_pomodoro) {
        connect(m_pomodoro, &PomodoroTimer::stateChanged, this, &MusicLinkManager::onPomodoroStateChanged);
        connect(m_pomodoro, &PomodoroTimer::paused, this, &MusicLinkManager::onPomodoroPaused);
        connect(m_pomodoro, &PomodoroTimer::resumed, this, &MusicLinkManager::onPomodoroResumed);
    }
}

int MusicLinkManager::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_links.size();
}

QVariant MusicLinkManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_links.size())
        return QVariant();

    const SavedMusicLink &link = m_links.at(index.row());

    switch (role) {
    case IdRole:
        return link.id;
    case LabelRole:
        return link.label;
    case UrlRole:
        return link.url;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> MusicLinkManager::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[LabelRole] = "label";
    roles[UrlRole] = "url";
    return roles;
}

void MusicLinkManager::addMusicLink(const QString &label, const QString &url)
{
    const QString trimmedLabel = label.trimmed();
    const QString trimmedUrl = url.trimmed();
    if (trimmedLabel.isEmpty() || trimmedUrl.isEmpty())
        return;

    if (!DatabaseManager::instance().addMusicLink(trimmedLabel, trimmedUrl))
        return;

    QSqlQuery query("SELECT last_insert_rowid()");
    if (!query.next())
        return;
    const int newId = query.value(0).toInt();

    beginInsertRows(QModelIndex(), m_links.size(), m_links.size());
    m_links.append({newId, trimmedLabel, trimmedUrl});
    endInsertRows();

    emit musicLinkAdded();
}

void MusicLinkManager::removeMusicLink(int index)
{
    if (index < 0 || index >= m_links.size())
        return;

    const SavedMusicLink link = m_links.at(index);

    if (!DatabaseManager::instance().removeMusicLink(link.id))
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_links.removeAt(index);
    endRemoveRows();

    emit musicLinkRemoved();

    // Clearing the selection is what stops it playing, if it was the one
    // currently sounding - see selectMusicLink().
    if (m_selectedLinkId == link.id)
        selectMusicLink(-1);
}

int MusicLinkManager::count() const
{
    return m_links.size();
}

int MusicLinkManager::selectedLinkId() const
{
    return m_selectedLinkId;
}

void MusicLinkManager::selectMusicLink(int id)
{
    if (id != -1) {
        const bool exists = std::any_of(m_links.begin(), m_links.end(),
                                         [id](const SavedMusicLink &link) { return link.id == id; });
        if (!exists)
            return;
    }

    if (m_selectedLinkId == id)
        return;

    m_selectedLinkId = id;
    persistSettings();
    emit selectedLinkIdChanged();

    // A Music Link picked - or cleared, including by deleting it - during a
    // live session takes effect immediately. Otherwise the old one plays on
    // until Stop while the UI shows the new one as selected.
    if (m_sessionActive) {
        startPlayback();
        return;
    }

    // Outside a session, resolve it now so that pressing Start is instant.
    prefetchSelectedLink();
}

qreal MusicLinkManager::volume() const
{
    return m_volume;
}

void MusicLinkManager::setVolume(qreal volume)
{
    const qreal clamped = std::clamp(volume, 0.0, 1.0);
    if (qFuzzyCompare(m_volume, clamped))
        return;

    m_volume = clamped;
    if (m_player)
        m_player->setVolume(m_volume);
    persistSettings();
    emit volumeChanged();
}

void MusicLinkManager::setPlaybackTuning(const PlaybackTuning &tuning)
{
    m_tuning = tuning;
}

void MusicLinkManager::loadFromDatabase()
{
    beginResetModel();

    m_links.clear();
    const QVector<QVariantMap> linkData = DatabaseManager::instance().loadMusicLinks();
    for (const QVariantMap &data : linkData) {
        m_links.append({data["id"].toInt(), data["label"].toString(), data["url"].toString()});
    }

    const QVariantMap settings = DatabaseManager::instance().loadMusicSettings();
    m_selectedLinkId = settings["selectedLinkId"].toInt();
    m_volume = settings["volume"].toDouble();

    endResetModel();

    // A previously-selected link may have been removed since the last run
    // (e.g. an interrupted shutdown); don't point at a link that no longer exists.
    if (m_selectedLinkId != -1) {
        const bool exists = std::any_of(m_links.begin(), m_links.end(),
                                         [this](const SavedMusicLink &link) { return link.id == m_selectedLinkId; });
        if (!exists) {
            m_selectedLinkId = -1;
            persistSettings();
        }
    }
}

void MusicLinkManager::persistSettings()
{
    DatabaseManager::instance().saveMusicSettings(m_selectedLinkId, m_volume);
}

bool MusicLinkManager::isPlaying() const
{
    return m_active && !m_paused;
}

QString MusicLinkManager::currentEntryUrl() const
{
    return m_currentEntryUrl;
}

QString MusicLinkManager::playbackError() const
{
    return m_playbackError;
}

void MusicLinkManager::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    notifyPlayingChanged();
}

void MusicLinkManager::setPaused(bool paused)
{
    if (m_paused == paused)
        return;
    m_paused = paused;
    notifyPlayingChanged();
}

void MusicLinkManager::notifyPlayingChanged()
{
    const bool nowPlaying = isPlaying();
    if (nowPlaying == m_reportedPlaying)
        return;
    m_reportedPlaying = nowPlaying;
    emit playingChanged();
}

void MusicLinkManager::setCurrentEntryUrl(const QString &url)
{
    if (m_currentEntryUrl == url)
        return;
    m_currentEntryUrl = url;
    emit currentEntryChanged();
}

void MusicLinkManager::setPlaybackError(const QString &message)
{
    if (m_playbackError == message)
        return;
    m_playbackError = message;
    emit playbackErrorChanged();
}

void MusicLinkManager::onPomodoroStateChanged(PomodoroTimer::State state)
{
    if (state == PomodoroTimer::Working) {
        m_sessionActive = true;
        startPlayback();
    } else if (state == PomodoroTimer::Idle) {
        m_sessionActive = false;
        stopPlayback();
    }
    // OnBreak: no reaction - music keeps playing through the break untouched.
}

void MusicLinkManager::onPomodoroPaused()
{
    if (!m_active || m_paused)
        return;

    setPaused(true);
    if (m_player)
        m_player->pause();
}

void MusicLinkManager::onPomodoroResumed()
{
    if (!m_active || !m_paused)
        return;

    setPaused(false);

    // An entry that ended or failed while we were paused was held back rather
    // than acted on; make good on it now that sound is wanted again.
    if (m_advanceWhenResumed) {
        m_advanceWhenResumed = false;
        skipToNextEntry();
        return;
    }

    if (m_player)
        m_player->resume();
}

void MusicLinkManager::startPlayback()
{
    clearPlaybackState();
    setPlaybackError(QString());

    if (m_selectedLinkId == -1 || !m_resolver)
        return;

    const QString selectedUrl = selectedLinkUrl();
    if (selectedUrl.isEmpty())
        return;

    if (startFromPrefetch())
        return;

    // Set the pending marker before asking: a resolver that answers
    // synchronously (the test fake does) re-enters onEntriesReady() from
    // inside this call, and must find a request it recognises.
    m_pendingMusicLinkUrl = selectedUrl;
    m_resolver->requestEntries(selectedUrl);
}

QString MusicLinkManager::selectedLinkUrl() const
{
    for (const SavedMusicLink &link : m_links) {
        if (link.id == m_selectedLinkId)
            return link.url;
    }
    return QString();
}

void MusicLinkManager::stopPlayback()
{
    clearPlaybackState();
    setPlaybackError(QString());
}

void MusicLinkManager::clearPlaybackState()
{
    // Bookkeeping first, collaborators last: a player that reacts to stop() by
    // emitting finished()/errorOccurred() then re-enters this object with
    // everything already cleared, where it is ignored, rather than acting on
    // half-cleared state.
    ++m_resolveGeneration;
    // An in-flight prefetch dies with the cancel() below, but whatever it had
    // already resolved is deliberately kept: Start is about to go looking for
    // it. Only discardPrefetch() throws the cache away.
    m_prefetchInFlight = false;
    ++m_prefetchGeneration;
    m_entries.clear();
    m_currentEntryIndex = -1;
    m_pendingMusicLinkUrl.clear();
    m_pendingEntryIndex = -1;
    m_pendingRetryUsed = false;
    m_sweepStart = 0;
    m_sweepTried = 0;
    m_consecutivePlaybackErrors = 0;
    m_advanceWhenResumed = false;
    m_currentEntryPlayingSince.invalidate();
    setPaused(false);
    setActive(false);
    setCurrentEntryUrl(QString());

    if (m_resolver)
        m_resolver->cancel();
    if (m_player)
        m_player->stop();
}

void MusicLinkManager::onEntriesReady(const QString &musicLinkUrl, const QVector<QString> &entries)
{
    if (m_prefetchInFlight) {
        onPrefetchEntriesReady(musicLinkUrl, entries);
        return;
    }

    if (m_pendingMusicLinkUrl.isEmpty() || musicLinkUrl != m_pendingMusicLinkUrl)
        return; // an answer to a question we've since stopped asking

    m_pendingMusicLinkUrl.clear();

    if (entries.isEmpty()) {
        // The Music Link itself couldn't be resolved at all (dead/private
        // playlist, network down) - distinct from "nothing selected", so say so.
        clearPlaybackState();
        setPlaybackError(tr("Couldn't play this Music Link."));
        return;
    }

    m_entries = entries;
    beginResolveSweep(0);
}

void MusicLinkManager::beginResolveSweep(int startIndex)
{
    if (m_entries.isEmpty())
        return;

    m_sweepStart = ((startIndex % m_entries.size()) + m_entries.size()) % m_entries.size();
    m_sweepTried = 0;
    requestEntryAt(m_sweepStart);
}

void MusicLinkManager::requestEntryAt(int index)
{
    if (!m_resolver || index < 0 || index >= m_entries.size())
        return;

    ++m_resolveGeneration; // strands any retry still queued for a previous entry
    m_pendingEntryIndex = index;
    m_pendingRetryUsed = false;
    m_resolver->requestStreamUrl(m_entries.at(index));
}

void MusicLinkManager::retryPendingEntry()
{
    if (!m_resolver || m_pendingEntryIndex < 0 || m_pendingEntryIndex >= m_entries.size())
        return;

    m_resolver->requestStreamUrl(m_entries.at(m_pendingEntryIndex));
}

void MusicLinkManager::onStreamUrlReady(const QString &entryUrl, const QString &streamUrl)
{
    if (m_prefetchInFlight) {
        onPrefetchStreamUrlReady(entryUrl, streamUrl);
        return;
    }

    if (m_pendingEntryIndex < 0 || m_pendingEntryIndex >= m_entries.size())
        return; // nothing outstanding; a late answer from a superseded sweep
    if (m_entries.at(m_pendingEntryIndex) != entryUrl)
        return;

    if (!streamUrl.isEmpty()) {
        playResolvedEntry(m_pendingEntryIndex, streamUrl);
        return;
    }

    if (!m_pendingRetryUsed) {
        m_pendingRetryUsed = true;
        // Wait before re-asking: a retry fired immediately lands inside the
        // same blip that just failed, and only costs another round-trip.
        const int generation = m_resolveGeneration;
        QTimer::singleShot(m_tuning.retryDelayMs, this, [this, generation]() {
            if (generation != m_resolveGeneration)
                return;
            retryPendingEntry();
        });
        return;
    }

    advanceSweepOrGiveUp();
}

void MusicLinkManager::playResolvedEntry(int index, const QString &streamUrl)
{
    m_pendingEntryIndex = -1;
    m_pendingRetryUsed = false;
    m_currentEntryIndex = index;

    // Commit every piece of state before the player is touched, so a player
    // that signals synchronously from play() finds this object consistent.
    setCurrentEntryUrl(m_entries.at(index));
    setPlaybackError(QString());
    setActive(true);
    m_currentEntryPlayingSince.restart();

    if (m_player)
        m_player->play(streamUrl);
}

void MusicLinkManager::advanceSweepOrGiveUp()
{
    ++m_sweepTried;

    if (m_sweepTried < m_entries.size()) {
        requestEntryAt((m_sweepStart + m_sweepTried) % m_entries.size());
        return;
    }

    // Every entry has now been tried once. A single-entry Music Link (nothing
    // to fall back to) surfaces a one-time inline error; a genuine playlist
    // just falls silent for the rest of the session - no error loop.
    const bool wasSingleEntry = (m_entries.size() == 1);
    clearPlaybackState();
    if (wasSingleEntry)
        setPlaybackError(tr("Couldn't play this Music Link."));
}

void MusicLinkManager::skipToNextEntry()
{
    if (m_entries.isEmpty())
        return;
    beginResolveSweep((m_currentEntryIndex + 1) % m_entries.size());
}

bool MusicLinkManager::prefetchIsUsable() const
{
    if (m_prefetch.linkId != m_selectedLinkId || m_prefetch.entries.isEmpty())
        return false;
    if (!m_prefetch.resolvedAt.isValid())
        return false;
    return m_prefetch.resolvedAt.msecsTo(QDateTime::currentDateTimeUtc()) < m_tuning.prefetchFreshnessMs;
}

bool MusicLinkManager::playbackResolveInFlight() const
{
    return !m_pendingMusicLinkUrl.isEmpty() || m_pendingEntryIndex >= 0;
}

bool MusicLinkManager::startFromPrefetch()
{
    if (!prefetchIsUsable()) {
        discardPrefetch();
        return false;
    }

    m_entries = m_prefetch.entries;

    // The stream URL is single-use - it is signed and expiring - but the entry
    // list is not, so a Start soon after still skips the slow enumeration.
    const QString streamUrl = m_prefetch.firstStreamUrl;
    m_prefetch.firstStreamUrl.clear();

    if (streamUrl.isEmpty()) {
        // Only the cheap half was missing (or it failed on its own). Enumerating
        // the playlist is the expensive part and it is already done, so this is
        // still worth far more than starting over.
        beginResolveSweep(0);
        return true;
    }

    m_sweepStart = 0;
    m_sweepTried = 0;
    playResolvedEntry(0, streamUrl);
    return true;
}

void MusicLinkManager::prefetchSelectedLink()
{
    discardPrefetch();

    if (!m_resolver || m_selectedLinkId == -1)
        return;

    const QString url = selectedLinkUrl();
    if (url.isEmpty())
        return;

    m_prefetch.linkId = m_selectedLinkId;
    m_prefetch.musicLinkUrl = url;

    // Let the selection settle before spending a process on it.
    const int generation = m_prefetchGeneration;
    QTimer::singleShot(m_tuning.prefetchDebounceMs, this, [this, generation]() {
        if (generation != m_prefetchGeneration)
            return;
        beginPrefetchRequest();
    });
}

void MusicLinkManager::beginPrefetchRequest()
{
    // Fires from a timer, so re-check rather than trusting what was true when
    // it was scheduled. The generation counter should already have stranded
    // this if playback claimed the resolver in the meantime; this is the
    // backstop for that reasoning being wrong.
    if (!m_resolver || m_prefetch.linkId != m_selectedLinkId || playbackResolveInFlight())
        return;

    m_prefetchInFlight = true;
    m_resolver->requestEntries(m_prefetch.musicLinkUrl);
}

void MusicLinkManager::onPrefetchEntriesReady(const QString &musicLinkUrl, const QVector<QString> &entries)
{
    if (!m_resolver || musicLinkUrl != m_prefetch.musicLinkUrl || entries.isEmpty()) {
        discardPrefetch();
        return;
    }

    m_prefetch.entries = entries;
    m_prefetch.resolvedAt = QDateTime::currentDateTimeUtc();
    m_resolver->requestStreamUrl(entries.first());
}

void MusicLinkManager::onPrefetchStreamUrlReady(const QString &entryUrl, const QString &streamUrl)
{
    m_prefetchInFlight = false;

    // A failed stream half is not worth discarding the entry list over - Start
    // will resolve the first entry itself and keep the enumeration.
    if (m_prefetch.entries.isEmpty() || entryUrl != m_prefetch.entries.first() || streamUrl.isEmpty())
        return;

    m_prefetch.firstStreamUrl = streamUrl;
}

void MusicLinkManager::discardPrefetch()
{
    ++m_prefetchGeneration; // strands a debounce still counting down

    if (m_prefetchInFlight) {
        m_prefetchInFlight = false;
        if (m_resolver)
            m_resolver->cancel();
    }

    m_prefetch = PrefetchedLink();
}

void MusicLinkManager::onEntryFinished()
{
    if (!m_active)
        return;

    if (m_paused) {
        m_advanceWhenResumed = true;
        return;
    }

    // A natural end is a successful playback, however many prior entries
    // were skipped for failing to resolve - reset the playback-error cap.
    m_consecutivePlaybackErrors = 0;
    skipToNextEntry();
}

void MusicLinkManager::onEntryPlaybackError()
{
    // Not active means there is nothing to skip to: the session stopped, or
    // the playlist already gave up and fell silent. A player that emits a late
    // or queued error afterwards must not repaint an error over an idle screen.
    if (!m_active)
        return;

    if (m_paused) {
        m_advanceWhenResumed = true;
        return;
    }

    // Distinct from a resolve failure (already capped by trying every entry
    // once per sweep): this is a resolved stream that fails during actual
    // playback. Without a cap, a stream that resolves fine but never plays
    // (bad codec, DRM, ...) would resolve-and-play-and-fail forever.
    //
    // An entry that played healthily before breaking starts a fresh run rather
    // than extending the previous one, so a long session of occasional
    // recovered hiccups never accumulates its way into giving up.
    if (m_currentEntryPlayingSince.isValid()
        && m_currentEntryPlayingSince.elapsed() >= m_tuning.healthyPlaybackMs) {
        m_consecutivePlaybackErrors = 0;
    }

    ++m_consecutivePlaybackErrors;
    if (m_consecutivePlaybackErrors > m_entries.size()) {
        clearPlaybackState();
        setPlaybackError(tr("Playback kept failing; stopping music for this session."));
        return;
    }

    skipToNextEntry();
}
