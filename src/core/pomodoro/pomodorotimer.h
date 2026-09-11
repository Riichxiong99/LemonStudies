#ifndef POMODOROTIMER_H
#define POMODOROTIMER_H

#include <QObject>

class QTimer;

class PomodoroTimer : public QObject {
    Q_OBJECT
    Q_PROPERTY(int timeRemaining READ timeRemaining NOTIFY timeRemainingChanged)
    Q_PROPERTY(State currentState READ currentState NOTIFY stateChanged)

public:
    enum State { Idle, Working, OnBreak };
    Q_ENUM(State)

    explicit PomodoroTimer(QObject *parent = nullptr);

    int timeRemaining() const;
    State currentState() const;
    bool isPaused() const;

    Q_INVOKABLE void startSession(int workDurationSeconds, int breakDurationSeconds);
    Q_INVOKABLE void stop();

    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();

    Q_INVOKABLE void killPomodoroView();


signals:
    void timeRemainingChanged();
    void stateChanged(State newState);
    // Distinct from stateChanged(): a paused session stays in Working/OnBreak,
    // so anything that has to stop and start alongside the countdown - music,
    // above all - has no state transition to react to without these.
    void paused();
    void resumed();
    void sessionEnded();
    void closePomodorotimer();

private slots:
    void onTimerTick();

private:
    QTimer *m_timer;
    int m_timeRemaining;
    int m_workDuration;
    int m_breakDuration;
    State m_currentState;
    bool m_paused;

    void setPaused(bool nowPaused);
    void transitionToWorking();
    void transitionToBreak();
    void transitionToIdle();
};

#endif // POMODOROTIMER_H
