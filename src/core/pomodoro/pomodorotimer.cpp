#include "pomodorotimer.h"
#include <QTimer>
#include <QDebug>

PomodoroTimer::PomodoroTimer(QObject *parent)
    : QObject(parent),
    m_timer(new QTimer(this)),
    m_timeRemaining(0),
    m_workDuration(0),
    m_breakDuration(0),
    m_currentState(Idle),
    m_paused(false)
{
    connect(m_timer, &QTimer::timeout, this, &PomodoroTimer::onTimerTick);
}

int PomodoroTimer::timeRemaining() const {
    return m_timeRemaining;
}

PomodoroTimer::State PomodoroTimer::currentState() const {
    return m_currentState;
}

bool PomodoroTimer::isPaused() const {
    return m_paused;
}

// Only an explicit pause()/resume() announces itself. Transitions clear the
// flag silently: a session on its way to Idle is not "resuming", and telling
// listeners otherwise makes them start audio a moment before it is stopped.
void PomodoroTimer::setPaused(bool nowPaused) {
    if (m_paused == nowPaused)
        return;
    m_paused = nowPaused;
    if (m_paused)
        emit paused();
    else
        emit resumed();
}

void PomodoroTimer::startSession(int workDurationSeconds, int breakDurationSeconds) {
    stop();
    m_workDuration = workDurationSeconds;
    m_breakDuration = breakDurationSeconds;
    transitionToWorking();
}

void PomodoroTimer::stop() {
    m_timer->stop();
    m_timeRemaining = 0;
    transitionToIdle();
    emit timeRemainingChanged();
}

void PomodoroTimer::pause() {
    if (m_currentState == Working || m_currentState == OnBreak) {
        m_timer->stop();
        setPaused(true);
    }
}

void PomodoroTimer::resume() {
    if ((m_currentState == Working || m_currentState == OnBreak) && m_timeRemaining > 0) {
        m_timer->start(1000);
        setPaused(false);
    }
}

void PomodoroTimer::onTimerTick() {
    if (m_timeRemaining > 0) {
        m_timeRemaining--;
        emit timeRemainingChanged();
    } else {
        if (m_currentState == Working) {
            transitionToBreak();
        } else if (m_currentState == OnBreak) {
            transitionToIdle();
            emit sessionEnded();
        }
    }
}

void PomodoroTimer::transitionToWorking() {
    m_paused = false;
    m_currentState = Working;
    m_timeRemaining = m_workDuration;
    emit stateChanged(m_currentState);
    emit timeRemainingChanged();
    m_timer->start(1000);
}

void PomodoroTimer::transitionToBreak() {
    m_paused = false;
    m_currentState = OnBreak;
    m_timeRemaining = m_breakDuration;
    emit stateChanged(m_currentState);
    emit timeRemainingChanged();
    m_timer->start(1000);
}

void PomodoroTimer::transitionToIdle() {
    m_paused = false;
    m_currentState = Idle;
    emit stateChanged(m_currentState);
}

void PomodoroTimer::killPomodoroView() {
    emit closePomodorotimer();
    qDebug() << "Testing";

}
