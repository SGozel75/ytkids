#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QTimer>

#include "timercontroller.h"

namespace YTKids {

class ArtworkFrame;
class PlayerWindow;

// ── Timer ring widget (forward declared in mainwindow) ────────────────────────
class TimerRing : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal pulse READ pulse WRITE setPulse)

public:
    explicit TimerRing(QWidget* parent = nullptr);

    void setTimes(int totalSec, int remainSec);

    qreal pulse() const     { return m_pulse; }
    void  setPulse(qreal v) { m_pulse = v; update(); }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    int     m_total     { 1 };
    int     m_remain    { 1 };
    qreal   m_pulse     { 0.0 };
    QTimer  m_pulseAnim;
    int     m_phase     { 0 };
    bool    m_warning   { false };
};

// ── MainWindow ────────────────────────────────────────────────────────────────
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event)  override;

private slots:
    void onTimerStateChanged(TimerController::State state);
    void onSessionTick(int remaining);
    void onCooldownTick(int remaining);
    void onWaitTick(int remaining);
    void onWarning(int minutesLeft);
    void onSessionExpired();
    void onVideoShouldFinish();
    void onCooldownFinished();
    void onScheduleWindowOpened();
    void onStartSession();
    void onStopSession();
    void onOpenParentalControls();
    void onBlockedChannel(const QString& name);

private:
    // ── UI builders ───────────────────────────────────────────────────────────
    void buildMenuBar();
    void buildTopBar();
    void buildSetupScreen();
    void buildPlayerScreen();
    void buildWaitScreen();
    void buildCooldownScreen();
    void buildLockedScreen();
    void buildGoodbyeScreen();

    void showScreen(int index);
    void updateTopBar(int remainSec, int totalSec);
    void showWarningDialog(int minutesLeft);
    QString formatTime(int seconds) const;

    // ── Screens (stacked) ─────────────────────────────────────────────────────
    enum Screen {
        ScreenSetup    = 0,
        ScreenPlayer   = 1,
        ScreenWait     = 2,
        ScreenCooldown = 3,
        ScreenLocked   = 4,
        ScreenGoodbye  = 5
    };

    QStackedWidget*     m_stack         { nullptr };

    // Setup screen
    ArtworkFrame*       m_setupArtwork  { nullptr };
    QPushButton*        m_startBtn      { nullptr };
    QLabel*             m_setupInfo     { nullptr };

    // Player screen
    QWidget*            m_topBar        { nullptr };
    TimerRing*          m_ring          { nullptr };
    QLabel*             m_timeLabel     { nullptr };
    QLabel*             m_statusLabel   { nullptr };
    QPushButton*        m_stopBtn       { nullptr };
    PlayerWindow*       m_player        { nullptr };

    // Wait / cooldown / locked screens
    ArtworkFrame*       m_waitArtwork       { nullptr };
    ArtworkFrame*       m_cooldownArtwork   { nullptr };
    ArtworkFrame*       m_lockedArtwork     { nullptr };

    // Controllers
    TimerController*    m_timer         { nullptr };
};

} // namespace YTKids
