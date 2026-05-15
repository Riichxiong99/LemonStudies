#include "pomodorotimer.h"
#include <QTimer>
#include <QDebug>

PomodoroTimer::PomodoroTimer(QObject *parent)
    : QObject(parent),
    m_timer(new QTimer(this)),
    m_timeRemaining(0),
    m_workDuration(0),
    m_breakDuration(0),
    m_currentState(Idle)
{
    connect(m_timer, &QTimer::timeout, this, &PomodoroTimer::onTimerTick);
}

int PomodoroTimer::timeRemaining() const {
    return m_timeRemaining;
}

PomodoroTimer::State PomodoroTimer::currentState() const {
    return m_currentState;
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
    }
}

void PomodoroTimer::resume() {
    if ((m_currentState == Working || m_currentState == OnBreak) && m_timeRemaining > 0) {
        m_timer->start(1000);
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
    m_currentState = Working;
    m_timeRemaining = m_workDuration;
    emit stateChanged(m_currentState);
    emit timeRemainingChanged();
    m_timer->start(1000);
}

void PomodoroTimer::transitionToBreak() {
    m_currentState = OnBreak;
    m_timeRemaining = m_breakDuration;
    emit stateChanged(m_currentState);
    emit timeRemainingChanged();
    m_timer->start(1000);
}

void PomodoroTimer::transitionToIdle() {
    m_currentState = Idle;
    emit stateChanged(m_currentState);
}

void PomodoroTimer::killPomodoroView() {
    emit closePomodorotimer();
    qDebug() << "Testing";

}
