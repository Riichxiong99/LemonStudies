#include "musiclinkmanager.h"
#include "../database/databasemanager.h"
#include "ytdlpmusiclinkresolver.h"
#include "qtmediamusicplayer.h"

#include <QSqlQuery>
#include <algorithm>

MusicLinkManager::MusicLinkManager(PomodoroTimer *pomodoro, QObject *parent)
    : MusicLinkManager(pomodoro, new YtDlpMusicLinkResolver(), new QtMediaMusicPlayer(), parent)
{
    m_ownsResolver = true;
    m_ownsPlayer = true;
}

MusicLinkManager::MusicLinkManager(PomodoroTimer *pomodoro, MusicLinkResolver *resolver, MusicPlayer *player,
                                    QObject *parent)
    : QAbstractListModel(parent),
    m_pomodoro(pomodoro),
    m_selectedLinkId(-1),
    m_volume(0.5),
    m_resolver(resolver),
    m_player(player),
    m_ownsResolver(false),
    m_ownsPlayer(false),
    m_currentEntryIndex(-1),
    m_playing(false),
    m_consecutivePlaybackErrors(0)
{
    DatabaseManager::instance().createMusicTables();
    loadFromDatabase();

    if (m_player) {
        m_player->setVolume(m_volume);
        connect(m_player, &MusicPlayer::finished, this, &MusicLinkManager::onEntryFinished);
        connect(m_player, &MusicPlayer::errorOccurred, this, &MusicLinkManager::onEntryPlaybackError);
    }

    if (m_pomodoro)
        connect(m_pomodoro, &PomodoroTimer::stateChanged, this, &MusicLinkManager::onPomodoroStateChanged);
}

MusicLinkManager::~MusicLinkManager()
{
    if (m_ownsPlayer)
        delete m_player;
    if (m_ownsResolver)
        delete m_resolver;
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

    const MusicLinkEntry &link = m_links.at(index.row());

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

    const MusicLinkEntry entry = m_links.at(index);

    if (!DatabaseManager::instance().removeMusicLink(entry.id))
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_links.removeAt(index);
    endRemoveRows();

    emit musicLinkRemoved();

    if (m_selectedLinkId == entry.id)
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
                                         [id](const MusicLinkEntry &link) { return link.id == id; });
        if (!exists)
            return;
    }

    if (m_selectedLinkId == id)
        return;

    m_selectedLinkId = id;
    persistSettings();
    emit selectedLinkIdChanged();
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
                                         [this](const MusicLinkEntry &link) { return link.id == m_selectedLinkId; });
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
    return m_playing;
}

QString MusicLinkManager::currentEntryUrl() const
{
    return m_currentEntryUrl;
}

QString MusicLinkManager::playbackError() const
{
    return m_playbackError;
}

void MusicLinkManager::pause()
{
    // Guarded on m_playing (not just m_player) so a stray Pause/Resume click
    // outside an active session - before Start, or after everything has
    // already stopped/gone silent - can't reach into the player at all.
    if (m_playing && m_player)
        m_player->pause();
}

void MusicLinkManager::resume()
{
    if (m_playing && m_player)
        m_player->resume();
}

void MusicLinkManager::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;
    m_playing = playing;
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
    if (state == PomodoroTimer::Working)
        startPlayback();
    else if (state == PomodoroTimer::Idle)
        stopPlayback();
    // OnBreak: no reaction - music keeps playing through the break untouched.
}

void MusicLinkManager::startPlayback()
{
    setPlaybackError(QString());
    m_entries.clear();
    m_currentEntryIndex = -1;
    m_consecutivePlaybackErrors = 0;

    if (m_selectedLinkId == -1) {
        setCurrentEntryUrl(QString());
        setPlaying(false);
        return;
    }

    QString selectedUrl;
    for (const MusicLinkEntry &link : m_links) {
        if (link.id == m_selectedLinkId) {
            selectedUrl = link.url;
            break;
        }
    }

    m_entries = m_resolver ? m_resolver->fetchEntries(selectedUrl) : QVector<QString>();
    if (m_entries.isEmpty()) {
        setCurrentEntryUrl(QString());
        setPlaying(false);
        // The Music Link itself couldn't be resolved at all (dead/private
        // playlist, network down) - distinct from "nothing selected", so say so.
        setPlaybackError(tr("Couldn't play this Music Link."));
        return;
    }

    tryPlayFromIndex(0);
}

void MusicLinkManager::stopPlayback()
{
    // Reset bookkeeping before touching the player, so a player that somehow
    // reacts to stop() synchronously (e.g. emits finished()) can't re-enter
    // this orchestration using half-cleared state.
    m_entries.clear();
    m_currentEntryIndex = -1;
    m_consecutivePlaybackErrors = 0;
    setCurrentEntryUrl(QString());
    setPlaying(false);
    setPlaybackError(QString());

    if (m_player)
        m_player->stop();
}

bool MusicLinkManager::tryPlayFromIndex(int startIndex)
{
    if (!m_resolver || !m_player || m_entries.isEmpty())
        return false;

    const int size = m_entries.size();
    for (int i = 0; i < size; ++i) {
        const int idx = (startIndex + i) % size;
        const QString &entryUrl = m_entries.at(idx);

        QString streamUrl = m_resolver->resolveStreamUrl(entryUrl);
        if (streamUrl.isEmpty())
            streamUrl = m_resolver->resolveStreamUrl(entryUrl); // silent retry-once

        if (!streamUrl.isEmpty()) {
            m_currentEntryIndex = idx;
            setCurrentEntryUrl(entryUrl);
            setPlaybackError(QString());
            m_player->play(streamUrl);
            setPlaying(true);
            return true;
        }
    }

    // Every entry failed. A single-entry Music Link (no other entry to fall
    // back to) surfaces a one-time inline error; a genuine playlist just
    // falls silent for the rest of the session - no error loop.
    const bool wasSingleEntry = (size == 1);
    m_entries.clear();
    m_currentEntryIndex = -1;
    setCurrentEntryUrl(QString());
    setPlaying(false);
    if (wasSingleEntry)
        setPlaybackError(tr("Couldn't play this Music Link."));
    return false;
}

void MusicLinkManager::skipToNextEntry()
{
    if (m_entries.isEmpty())
        return;
    const int nextIndex = (m_currentEntryIndex + 1) % m_entries.size();
    tryPlayFromIndex(nextIndex);
}

void MusicLinkManager::onEntryFinished()
{
    // A natural end is a successful playback, however many prior entries
    // were skipped for failing to resolve - reset the playback-error cap.
    m_consecutivePlaybackErrors = 0;
    skipToNextEntry();
}

void MusicLinkManager::onEntryPlaybackError()
{
    // Distinct from a resolve failure (already capped by trying every entry
    // once in tryPlayFromIndex): this is a resolved stream that fails during
    // actual playback. Without a cap, a stream that resolves fine but never
    // plays (bad codec, DRM, ...) would resolve-and-play-and-fail forever.
    ++m_consecutivePlaybackErrors;
    if (m_consecutivePlaybackErrors > m_entries.size()) {
        if (m_player)
            m_player->stop();
        m_entries.clear();
        m_currentEntryIndex = -1;
        m_consecutivePlaybackErrors = 0;
        setCurrentEntryUrl(QString());
        setPlaying(false);
        setPlaybackError(tr("Playback kept failing; stopping music for this session."));
        return;
    }

    skipToNextEntry();
}
