#pragma once

#include <QObject>
#include <QTimer>
#include <QTime>

namespace YTKids {

class TimerController : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Idle,           // not started
        Waiting,        // outside allowed window — counting down to start
        Running,        // session active
        Warning5,       // 5 min remaining
        Warning1,       // 1 min remaining
        GraceFinish,    // time up, finishing current video
        Cooldown,       // session ended, no-screen cooling down
        Locked          // cooldown done, waiting for next allowed window
    };
    Q_ENUM(State)

    explicit TimerController(QObject* parent = nullptr);

    // ── Control ───────────────────────────────────────────────────────────────
    void startSession();
    void stopSession();
    void pauseSession();
    void resumeSession();

    // ── Video remaining — fed from WebEngine JS poll ───────────────────────
    void updateVideoRemaining(int seconds);     // called every 2s from player

    // ── Accessors ─────────────────────────────────────────────────────────────
    State   state()            const { return m_state; }
    int     sessionRemaining() const { return m_sessionRemain; }
    int     cooldownRemaining() const { return m_cooldownRemain; }
    int     waitRemaining()    const { return m_waitRemain; }
    bool    isPaused()         const { return m_paused; }

signals:
    void stateChanged(State newState);
    void sessionTick(int remainingSeconds);         // every second
    void cooldownTick(int remainingSeconds);        // every second
    void waitTick(int remainingSeconds);            // every second while waiting
    void warning(int minutesLeft);                  // 5 and 1
    void sessionExpired();                          // hard stop now
    void videoShouldFinish();                       // let current video end
    void cooldownFinished();                        // cooldown over
    void scheduleWindowOpened();                    // allowed window just started

private slots:
    void onSessionTick();
    void onCooldownTick();
    void onWaitTick();
    void onScheduleCheck();

private:
    void setState(State s);
    void startCooldown();
    void startWaiting();
    void evaluateTimeUp();      // grace period decision point
    int  secondsUntilWindow() const;

    State   m_state           { State::Idle };
    int     m_sessionTotal    { 1800 };
    int     m_sessionRemain   { 1800 };
    int     m_cooldownTotal   { 1800 };
    int     m_cooldownRemain  { 1800 };
    int     m_waitRemain      { 0 };
    int     m_gracePeriod     { 300 };
    int     m_videoRemaining  { -1 };   // -1 = unknown
    bool    m_warned5         { false };
    bool    m_warned1         { false };
    bool    m_paused          { false };

    QTimer  m_sessionTimer;
    QTimer  m_cooldownTimer;
    QTimer  m_waitTimer;
    QTimer  m_scheduleTimer;    // checks every 30s if window opened
};

} // namespace YTKids
