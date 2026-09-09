#ifndef MUSICLINKMANAGER_H
#define MUSICLINKMANAGER_H

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVector>

#include "../pomodoro/pomodorotimer.h"

// Manages the user's saved Music Links (labeled YouTube URLs) for the
// Pomodoro view: adding/removing them, tracking which one is selected, and
// the playback volume. Selection and volume persist across restarts.
//
// Takes PomodoroTimer* (like BlockingManager) so a later ticket can react to
// session state without changing this constructor's signature; this ticket
// does not yet play any audio.
class MusicLinkManager : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int selectedLinkId READ selectedLinkId WRITE selectMusicLink NOTIFY selectedLinkIdChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)

public:
    explicit MusicLinkManager(PomodoroTimer *pomodoro, QObject *parent = nullptr);

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

signals:
    void musicLinkAdded();
    void musicLinkRemoved();
    void selectedLinkIdChanged();
    void volumeChanged();

private:
    struct MusicLinkEntry {
        int id;
        QString label;
        QString url;
    };

    void loadFromDatabase();
    void persistSettings();

    PomodoroTimer *m_pomodoro;
    QVector<MusicLinkEntry> m_links;
    int m_selectedLinkId;
    qreal m_volume;
};

#endif // MUSICLINKMANAGER_H
