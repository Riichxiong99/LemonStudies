#ifndef STREAKSMANAGER_H
#define STREAKSMANAGER_H


#include <QAbstractListModel>
#include <QObject>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <QTimer>

#include "streaks.h"

class streaksManager: public QAbstractListModel
{

    Q_OBJECT

    Q_PROPERTY(int totalStreaks READ totalStreaks NOTIFY totalStreaksChanged)
    Q_PROPERTY(int activeStreaks READ activeStreaks NOTIFY activeStreaksChanged)

private:
    QVector<Streaks*> m_streaks;
    QTimer *m_dailyResetTimer;

    void updateStats();
    void loadStreaksFromDatabase();
    void saveStreakToDatabase(Streaks *streak);
    void updateStreakInDatabase(Streaks *streak);
    void deleteStreakFromDatabase(int id);
    void setupDailyResetTimer();
    void checkAndResetExpiredStreaks();

public:
    explicit streaksManager(QObject *parent = nullptr);

    enum StreakRoles{
        TitleRole = Qt::UserRole + 1,
        StreakDurationRole,
        BestStreakRole,
        LastActivityRole,
        IsActiveTodayRole,
        IsStreakBrokenRole,
        DaysSinceLastActivityRole,
        IsBestStreakZeroRole
    };

    // REQUIRED METHODS FOR QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addStreak(const QString &title);
    Q_INVOKABLE void removeStreak(int index);
    Q_INVOKABLE void incrementStreak(int index);
    Q_INVOKABLE void resetStreak(int index);
    Q_INVOKABLE int count() const;
    Q_INVOKABLE void killStreaksView();

    int totalStreaks() const;
    int activeStreaks() const;

signals:
    void streakAdded();
    void streakRemoved();
    void streakUpdated();
    void totalStreaksChanged();
    void activeStreaksChanged();
    void closeStreaksView();



};

#endif // STREAKSMANAGER_H
