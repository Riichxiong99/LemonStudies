#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H


#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager& instance();

    bool openDatabase();
    bool createTables();
    QSqlDatabase database() const;

    Q_INVOKABLE bool createStreaksTable();
    Q_INVOKABLE bool saveStreak(const QString &title, int streakDuration, int bestStreak, const QDateTime &lastActivity);
    Q_INVOKABLE QVector<QVariantMap> loadAllStreaks();
    Q_INVOKABLE bool updateStreak(int id, const QString &title, int streakDuration, int bestStreak, const QDateTime &lastActivity);
    Q_INVOKABLE bool deleteStreak(int id);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    QSqlDatabase m_database;




};


#endif // DATABASEMANAGER_H
