#include "timercontroller.h"
#include "config.h"

#include <QTime>

namespace YTKids {

TimerController::TimerController(QObject* parent)
    : QObject(parent)
{
    // Session tick — every second
    m_sessionTimer.setInterval(1000);
    m_sessionTimer.setSingleShot(false);
    connect(&m_sessionTimer, &QTimer::timeout,
            this, &TimerController::onSessionTick);

    // Cooldown tick — every second
    m_cooldownTimer.setInterval(1000);
    m_cooldownTimer.setSingleShot(false);
    connect(&m_cooldownTimer, &QTimer::timeout,
            this, &TimerController::onCooldownTick);

    // Wait tick — every second
    m_waitTimer.setInterval(1000);
    m_waitTimer.setSingleShot(false);
    connect(&m_waitTimer, &QTimer::timeout,
            this, &TimerController::onWaitTick);

    // Schedule check — every 30 seconds
    m_scheduleTimer.setInterval(30000);
    m_scheduleTimer.setSingleShot(false);
    connect(&m_scheduleTimer, &QTimer::timeout,
            this, &TimerController::onScheduleCheck);
}

// ── Control ───────────────────────────────────────────────────────────────────
void TimerController::startSession()
{
    Config& cfg = Config::instance();

    m_sessionTotal   = cfg.sessionDuration();
    m_sessionRemain  = m_sessionTotal;
    m_cooldownTotal  = cfg.cooldownDuration();
    m_gracePeriod    = cfg.gracePeriod();
    m_warned5        = false;
    m_warned1        = false;
    m_paused         = false;
    m_videoRemaining = -1;

    // Check schedule first
    if (cfg.scheduleEnabled() && !cfg.isWithinAllowedWindow()) {
        startWaiting();
        return;
    }

    m_sessionTimer.start();
    setState(State::Running);
    emit sessionTick(m_sessionRemain);
}

void TimerController::stopSession()
{
    m_sessionTimer.stop();
    m_cooldownTimer.stop();
    m_waitTimer.stop();
    m_scheduleTimer.stop();
    m_paused = false;
    setState(State::Idle);
}

void TimerController::pauseSession()
{
    if (m_state != State::Running &&
        m_state != State::Warning5 &&
        m_state != State::Warning1)
        return;

    m_sessionTimer.stop();
    m_paused = true;
}

void TimerController::resumeSession()
{
    if (!m_paused) return;
    m_sessionTimer.start();
    m_paused = false;
}

// ── Video remaining update (from WebEngine poll) ───────────────────────────────
void TimerController::updateVideoRemaining(int seconds)
{
    m_videoRemaining = seconds;

    // If we're in GraceFinish mode and video just ended
    if (m_state == State::GraceFinish && seconds >= 0 && seconds < 2) {
        m_sessionTimer.stop();
        emit sessionExpired();
        startCooldown();
    }
}

// ── Session tick ──────────────────────────────────────────────────────────────
void TimerController::onSessionTick()
{
    if (m_sessionRemain > 0)
        --m_sessionRemain;

    emit sessionTick(m_sessionRemain);

    // Warning at 5 min
    if (!m_warned5 && m_sessionRemain <= 300) {
        m_warned5 = true;
        setState(State::Warning5);
        emit warning(5);
    }

    // Warning at 1 min
    if (!m_warned1 && m_sessionRemain <= 60) {
        m_warned1 = true;
        setState(State::Warning1);
        emit warning(1);
    }

    // Time up
    if (m_sessionRemain <= 0) {
        m_sessionTimer.stop();
        evaluateTimeUp();
    }
}

// ── Grace period decision ─────────────────────────────────────────────────────
void TimerController::evaluateTimeUp()
{
    if (m_videoRemaining < 0 || m_videoRemaining > m_gracePeriod) {
        // Unknown or too much left — hard stop
        emit sessionExpired();
        startCooldown();
    } else {
        // Let the video finish
        setState(State::GraceFinish);
        emit videoShouldFinish();
        // updateVideoRemaining() will trigger the actual stop
    }
}

// ── Cooldown ──────────────────────────────────────────────────────────────────
void TimerController::startCooldown()
{
    m_cooldownRemain = m_cooldownTotal;
    m_cooldownTimer.start();
    setState(State::Cooldown);
    emit cooldownTick(m_cooldownRemain);
}

void TimerController::onCooldownTick()
{
    if (m_cooldownRemain > 0)
        --m_cooldownRemain;

    emit cooldownTick(m_cooldownRemain);

    if (m_cooldownRemain <= 0) {
        m_cooldownTimer.stop();
        emit cooldownFinished();

        // Check if still within allowed window
        Config& cfg = Config::instance();
        if (cfg.scheduleEnabled() && !cfg.isWithinAllowedWindow()) {
            setState(State::Locked);
            m_scheduleTimer.start();
        } else {
            setState(State::Idle);
        }
    }
}

// ── Waiting for schedule window ───────────────────────────────────────────────
void TimerController::startWaiting()
{
    m_waitRemain = secondsUntilWindow();
    m_waitTimer.start();
    m_scheduleTimer.start();
    setState(State::Waiting);
    emit waitTick(m_waitRemain);
}

void TimerController::onWaitTick()
{
    if (m_waitRemain > 0)
        --m_waitRemain;

    emit waitTick(m_waitRemain);
}

void TimerController::onScheduleCheck()
{
    if (Config::instance().isWithinAllowedWindow()) {
        m_waitTimer.stop();
        m_scheduleTimer.stop();
        emit scheduleWindowOpened();

        if (m_state == State::Waiting) {
            // Auto-start session
            m_sessionTimer.start();
            setState(State::Running);
            emit sessionTick(m_sessionRemain);
        } else if (m_state == State::Locked) {
            setState(State::Idle);
        }
    } else {
        // Recalculate wait time (handles day rollover)
        m_waitRemain = secondsUntilWindow();
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────
void TimerController::setState(State s)
{
    if (m_state == s) return;
    m_state = s;
    emit stateChanged(s);
}

int TimerController::secondsUntilWindow() const
{
    const QTime now   = QTime::currentTime();
    const QTime start = Config::instance().allowedStart();

    int secs = now.secsTo(start);
    if (secs < 0)
        secs += 86400;  // next day

    return secs;
}

} // namespace YTKids
