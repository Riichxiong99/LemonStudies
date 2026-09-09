#include "musiclinkmanager.h"
#include "../database/databasemanager.h"

#include <QSqlQuery>
#include <algorithm>

MusicLinkManager::MusicLinkManager(PomodoroTimer *pomodoro, QObject *parent)
    : QAbstractListModel(parent),
    m_pomodoro(pomodoro),
    m_selectedLinkId(-1),
    m_volume(0.5)
{
    DatabaseManager::instance().createMusicTables();
    loadFromDatabase();
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
