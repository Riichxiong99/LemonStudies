#include "streaksmanager.h"
#include "../database/databasemanager.h"

#include <QSqlQuery>

streaksManager::streaksManager(QObject *parent)
    : QAbstractListModel(parent)
{
    DatabaseManager::instance().createStreaksTable();
    loadStreaksFromDatabase();
    setupDailyResetTimer();
}

int streaksManager::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid()){
        return 0;
    }
    return m_streaks.size();
}


QVariant streaksManager::data(const QModelIndex &index, int role) const
{

    if(!index.isValid() || index.row() < 0 || index.row() >= m_streaks.size()){
        return QVariant();
    }

    Streaks *streak = m_streaks.at(index.row());

    switch(role){
    case TitleRole:
        return streak->title();
    case StreakDurationRole:
        return streak->streakDuration();
    case BestStreakRole:
        return streak->bestStreak();
    case LastActivityRole:
        return streak->lastActivity();
    case IsActiveTodayRole:
        return streak->isActiveToday();
    case IsStreakBrokenRole:
        return streak->isStreakBroken();
    case DaysSinceLastActivityRole:
        return streak->daysSinceLastActivity();
    case IsBestStreakZeroRole:
        return streak->isBestStreakZero();
    default:
        return QVariant();
    }
}

bool streaksManager::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_streaks.size())
        return false;

    Streaks *streak = m_streaks.at(index.row());
    bool changed = false;

    switch (role) {
    case TitleRole:
        streak->setTitle(value.toString());
        changed = true;
        break;
    case StreakDurationRole:
        streak->setStreakDuration(value.toInt());
        changed = true;
        break;
    default:
        break;
    }

    if (changed) {
        updateStreakInDatabase(streak);
        emit dataChanged(index, index, QVector<int>() << role);
        emit streakUpdated();
        emit activeStreaksChanged();
        return true;
    }
    return false;
}

Qt::ItemFlags streaksManager::flags(const QModelIndex &index) const
{
    if(!index.isValid()){
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEditable | QAbstractListModel::flags(index);
}

QHash<int, QByteArray> streaksManager::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TitleRole] = "title";
    roles[StreakDurationRole] = "streakDuration";
    roles[BestStreakRole] = "bestStreak";
    roles[LastActivityRole] = "lastActivity";
    roles[IsActiveTodayRole] = "isActiveToday";
    roles[IsStreakBrokenRole] = "isStreakBroken";
    roles[DaysSinceLastActivityRole] = "daysSinceLastActivity";
    roles[IsBestStreakZeroRole] = "isBestStreakZero";
    return roles;
}

void streaksManager::addStreak(const QString &title)
{
    // Save to database first
    if (!DatabaseManager::instance().saveStreak(title, 0, 0, QDateTime())) {
        qDebug() << "Failed to save streak to database";
        return;
    }

    QVector<QVariantMap> allStreaks = DatabaseManager::instance().loadAllStreaks();
    if (allStreaks.isEmpty()) return;

    QVariantMap newStreakData;
    for (auto it = allStreaks.rbegin(); it != allStreaks.rend(); ++it) {
        if (it->value("title").toString() == title) {
            newStreakData = *it;
            break;
        }
    }

    beginInsertRows(QModelIndex(), m_streaks.size(), m_streaks.size());
    Streaks *newStreak = new Streaks(title, this);
    newStreak->setId(newStreakData["id"].toInt());
    m_streaks.append(newStreak);
    endInsertRows();

    emit streakAdded();
    emit totalStreaksChanged();
    emit activeStreaksChanged();
}

void streaksManager::removeStreak(int index)
{
    if (index < 0 || index >= m_streaks.size())
        return;

    Streaks *streak = m_streaks.at(index);
    int streakId = streak->id();

    // Delete from database first
    deleteStreakFromDatabase(streakId);

    // Then remove from model
    beginRemoveRows(QModelIndex(), index, index);
    m_streaks.removeAt(index);
    delete streak;
    endRemoveRows();

    emit streakRemoved();
    emit totalStreaksChanged();
    emit activeStreaksChanged();
}


void streaksManager::incrementStreak(int index)
{
    if (index < 0 || index >= m_streaks.size())
        return;

    Streaks *streak = m_streaks.at(index);
    streak->incrementStreakDuration();

    // Update in database
    DatabaseManager::instance().updateStreak(
        streak->id(),
        streak->title(),
        streak->streakDuration(),
        streak->bestStreak(),
        streak->lastActivity()
        );

    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex);
    emit streakUpdated();
    emit activeStreaksChanged();
}

void streaksManager::resetStreak(int index)
{
    if (index < 0 || index >= m_streaks.size())
        return;

    Streaks *streak = m_streaks.at(index);
    streak->resetStreakDuration();

    // Update in database
    updateStreakInDatabase(streak);

    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex);
    emit streakUpdated();
    emit activeStreaksChanged();
}

int streaksManager::count() const
{
    return m_streaks.size();
}

void streaksManager::killStreaksView()
{
    emit closeStreaksView();
}

int streaksManager::totalStreaks() const
{
    return m_streaks.size();
}

int streaksManager::activeStreaks() const
{
    int count = 0;
    for (const Streaks *streak : m_streaks) {
        if (streak->isActiveToday()) {
            count++;
        }
    }
    return count;
}

void streaksManager::updateStats()
{
    emit totalStreaksChanged();
    emit activeStreaksChanged();
}







void streaksManager::loadStreaksFromDatabase()
{
    beginResetModel();

    qDeleteAll(m_streaks);
    m_streaks.clear();

    // Load from database
    QVector<QVariantMap> streakData = DatabaseManager::instance().loadAllStreaks();

    for (const QVariantMap &data : streakData) {
        Streaks *streak = new Streaks(data["title"].toString(), this);
        streak->setId(data["id"].toInt());
        streak->setStreakDuration(data["streakDuration"].toInt());
        streak->setLastActivity(data["lastActivity"].toDateTime());
        streak->setBestStreak(data["bestStreak"].toInt());

        m_streaks.append(streak);
    }

    endResetModel();
}


void streaksManager::saveStreakToDatabase(Streaks *streak)
{
    if (!streak) {
        qDebug() << "Error: Cannot save null streak to database";
        return;
    }

    if (streak->id() != -1) {
        qDebug() << "Streak already exists in database with ID:" << streak->id();
        return;
    }

    // Save to database
    bool success = DatabaseManager::instance().saveStreak(
        streak->title(),
        streak->streakDuration(),
        streak->bestStreak(),
        streak->lastActivity()
        );

    if (!success) {
        qDebug() << "Failed to save streak to database:" << streak->title();
        return;
    }

    QSqlQuery query("SELECT last_insert_rowid()");
    if (query.next()) {
        int newId = query.value(0).toInt();
        streak->setId(newId);
        qDebug() << "Successfully saved streak" << streak->title() << "with ID:" << newId;
    } else {
        qDebug() << "Error: Could not retrieve new streak ID";
    }
}



void streaksManager::updateStreakInDatabase(Streaks *streak)
{
    if (!streak) {
        qDebug() << "Error: Cannot update null streak in database";
        return;
    }

    if (streak->id() <= 0) {
        qDebug() << "Warning: Streak" << streak->title() << "has no database ID. Use saveStreakToDatabase() first.";
        return;
    }

    qDebug() << "Updating streak in database:" << streak->title()
             << "Duration:" << streak->streakDuration()
             << "Best:" << streak->bestStreak();

    bool success = DatabaseManager::instance().updateStreak(
        streak->id(),
        streak->title(),
        streak->streakDuration(),
        streak->bestStreak(),
        streak->lastActivity()
        );

    if (success) {
        qDebug() << "✓ Streak updated successfully in database";
    } else {
        qDebug() << "✗ Failed to update streak in database";
    }
}

void streaksManager::deleteStreakFromDatabase(int id)
{
    if (id <= 0) {
        qDebug() << "Error: Cannot delete streak with invalid ID:" << id;
        return;
    }

    qDebug() << "Attempting to delete streak from database with ID:" << id;

    // Try to delete from database
    bool success = DatabaseManager::instance().deleteStreak(id);

    if (success) {
        qDebug() << "✓ Successfully deleted streak with ID:" << id << "from database";
    } else {
        qDebug() << "✗ Failed to delete streak with ID:" << id << "from database";
        qDebug() << "  Note: Streak may not exist in database or database error occurred";
    }
}

#include <QTimer>
#include <QDateTime>


void streaksManager::setupDailyResetTimer() {
    m_dailyResetTimer = new QTimer(this);

    m_dailyResetTimer->setInterval(3600000);

    connect(m_dailyResetTimer, &QTimer::timeout,
            this, &streaksManager::checkAndResetExpiredStreaks);

    m_dailyResetTimer->start();

    checkAndResetExpiredStreaks();
}

void streaksManager::checkAndResetExpiredStreaks() {
    QDateTime now = QDateTime::currentDateTime();

    for (int i = 0; i < m_streaks.size(); ++i) {
        Streaks *streak = m_streaks[i];

        if (streak->lastActivity().secsTo(now) > 86400) { // 86400 seconds = 24 hours
            if (streak->streakDuration() > 0) {
                streak->setStreakDuration(0);  // Reset to 0
                updateStreakInDatabase(streak);

                // Notify UI of change
                QModelIndex index = createIndex(i, 0);
                emit dataChanged(index, index);
            }
        }
    }

    // Update stats
    emit activeStreaksChanged();
}

