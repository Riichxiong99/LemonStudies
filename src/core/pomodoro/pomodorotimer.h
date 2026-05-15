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

    Q_INVOKABLE void startSession(int workDurationSeconds, int breakDurationSeconds);
    Q_INVOKABLE void stop();

    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();

    Q_INVOKABLE void killPomodoroView();


signals:
    void timeRemainingChanged();
    void stateChanged(State newState);
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

    void transitionToWorking();
    void transitionToBreak();
    void transitionToIdle();
};

#endif // POMODOROTIMER_H
