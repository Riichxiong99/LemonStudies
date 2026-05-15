#ifndef STREAKS_H
#define STREAKS_H

#include <QObject>
#include <QString>
#include <QDateTime>

class Streaks : public QObject
{
    Q_OBJECT

private:
    QString m_title;
    int m_streakDuration;
    int m_bestStreak;
    QDateTime m_lastActivity;
    int m_id;

public:

    // Default constructor
    explicit Streaks(const QString &title,QObject *parent = nullptr);

    // Getter functions.
    QString title() const;
    int streakDuration() const;

    // Setters
    void setTitle(const QString &title);
    void incrementStreakDuration();
    void resetStreakDuration();
    void setStreakDuration(int duration);

    // Functions.
    QDateTime lastActivity() const;
    void setLastActivity(const QDateTime &dateTime);
    bool isActiveToday() const;
    bool isStreakBroken() const;
    int daysSinceLastActivity() const;
    int bestStreak() const;
    void updateBestStreak();
    int id() const;
    void setId(int id);
    void setBestStreak(int bestStreak);
    bool isBestStreakZero() const;


signals:
    void titleChanged();
    void streakDurationChanged();

};


#endif // STREAKS_H
