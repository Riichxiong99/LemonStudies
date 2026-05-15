#include "streaks.h"

#include <QObject>
#include <QDateTime>
#include <algorithm>

// Default Constructor
Streaks::Streaks(const QString &title,QObject *parent)

    : QObject(parent),
    m_title(title),
    m_streakDuration(0),
    m_bestStreak(0),
    m_lastActivity(),
    m_id(-1)
{
}

// Getter Methods
QString Streaks::title() const {
    return m_title;
}


int Streaks::streakDuration() const{
    return m_streakDuration;
}

// Setter Methods

void Streaks::setTitle(const QString &title){

    if(m_title != title){
        m_title = title;
        emit titleChanged();
    }

}

void Streaks::incrementStreakDuration(){
    m_streakDuration++;
    m_lastActivity = QDateTime::currentDateTime();
    updateBestStreak();
    emit streakDurationChanged();
}

void Streaks::resetStreakDuration(){
    m_streakDuration = 0;
    emit streakDurationChanged();
}

void Streaks::setStreakDuration(int duration){
    if(m_streakDuration != duration){
        m_streakDuration = duration;
        emit streakDurationChanged();
    }
}
QDateTime Streaks::lastActivity() const {
    return m_lastActivity;
}
void Streaks::setLastActivity(const QDateTime &dateTime){
    m_lastActivity = dateTime;
}

bool Streaks::isActiveToday() const{

    if(!m_lastActivity.isValid()){
        return false;
    }
    QDate today = QDate::currentDate();
    QDate lastActive = m_lastActivity.date();

    return today == lastActive;

}
bool Streaks::isStreakBroken() const {

    if(!m_lastActivity.isValid()){
        return true;
    }

    QDate today = QDate::currentDate();
    QDate lastActive = m_lastActivity.date();

    int difference = lastActive.daysTo(today);

    return difference > 1;

}
int Streaks::daysSinceLastActivity() const{

    if(!m_lastActivity.isValid()){
        return -1;
    }

    QDate today = QDate::currentDate();
    QDate lastActive = m_lastActivity.date();

    int difference = lastActive.daysTo(today);

    return difference;

}
int Streaks::bestStreak() const {

    return m_bestStreak;
}
void Streaks::updateBestStreak(){

    m_bestStreak = std::max(m_bestStreak,m_streakDuration);

}

int Streaks::id() const {
    return m_id;
}

void Streaks::setId(int id) {
    m_id = id;
}

void Streaks::setBestStreak(int bestStreak){
    m_bestStreak = bestStreak;
}

bool Streaks::isBestStreakZero() const
{
    return m_bestStreak == 0;
}
