#include "databasemanager.h"
#include <QSqlQuery>      // If not already there
#include <QSqlError>      // If not already there
#include <QDebug>         // If not already there
#include <QDateTime>      // ← Add this line
#include <QVariant>       // ← Add this too
#include <QVariantMap>    // ← And this

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName("lemonstudys.db");
}

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::openDatabase()
{
    if (!m_database.open()) {
        qDebug() << "Error: Could not open database:" << m_database.lastError().text();
        return false;
    }

    qDebug() << "Database opened successfully";
    return createTables();
}

bool DatabaseManager::createTables()
{
    QSqlQuery query;

    // Create todos table with your existing fields
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS todos ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL,"
        "description TEXT,"
        "completed BOOLEAN DEFAULT 0,"
        "priority INTEGER DEFAULT 1,"
        "due_date DATETIME,"
        "created_date DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    if (!success) {
        qDebug() << "Error creating todos table:" << query.lastError().text();
        return false;
    }

    qDebug() << "Tables created successfully";
    return true;
}

QSqlDatabase DatabaseManager::database() const
{
    return m_database;
}

bool DatabaseManager::createStreaksTable()
{
    QSqlQuery query;
    QString createTable = R"(
        CREATE TABLE IF NOT EXISTS streaks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            streak_duration INTEGER DEFAULT 0,
            best_streak INTEGER DEFAULT 0,
            last_activity DATETIME,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!query.exec(createTable)) {
        qDebug() << "Error creating streaks table:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::saveStreak(const QString &title, int streakDuration, int bestStreak, const QDateTime &lastActivity)
{
    QSqlQuery query;
    query.prepare("INSERT INTO streaks (title, streak_duration, best_streak, last_activity) "
                  "VALUES (:title, :streak_duration, :best_streak, :last_activity)");
    query.bindValue(":title", title);
    query.bindValue(":streak_duration", streakDuration);
    query.bindValue(":best_streak", bestStreak);
    query.bindValue(":last_activity", lastActivity.isValid() ? lastActivity : QVariant());

    if (!query.exec()) {
        qDebug() << "Error saving streak:" << query.lastError().text();
        return false;
    }
    return true;
}

QVector<QVariantMap> DatabaseManager::loadAllStreaks()
{
    QVector<QVariantMap> streaks;
    QSqlQuery query("SELECT id, title, streak_duration, best_streak, last_activity FROM streaks ORDER BY created_at");

    while (query.next()) {
        QVariantMap streak;
        streak["id"] = query.value(0).toInt();
        streak["title"] = query.value(1).toString();
        streak["streakDuration"] = query.value(2).toInt();
        streak["bestStreak"] = query.value(3).toInt();
        streak["lastActivity"] = query.value(4).toDateTime();
        streaks.append(streak);
    }

    return streaks;
}

bool DatabaseManager::updateStreak(int id, const QString &title, int streakDuration, int bestStreak, const QDateTime &lastActivity)
{
    QSqlQuery query;
    query.prepare("UPDATE streaks SET title = :title, streak_duration = :streak_duration, "
                  "best_streak = :best_streak, last_activity = :last_activity WHERE id = :id");
    query.bindValue(":title", title);
    query.bindValue(":streak_duration", streakDuration);
    query.bindValue(":best_streak", bestStreak);
    query.bindValue(":last_activity", lastActivity.isValid() ? lastActivity : QVariant());
    query.bindValue(":id", id);

    return query.exec();
}

bool DatabaseManager::deleteStreak(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM streaks WHERE id = :id");
    query.bindValue(":id", id);
    return query.exec();
}
