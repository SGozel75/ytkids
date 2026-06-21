#include "mainwindow.h"
#include "artworkframe.h"
#include "playerwindow.h"
#include "parentalcontrol.h"
#include "config.h"

#include <QPainter>
#include <QPen>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QTimer>
#include <cmath>

namespace YTKids {

// ── Palette constants ─────────────────────────────────────────────────────────
static const QColor C_BG      { "#1A0A35" };
static const QColor C_CARD    { "#2D1654" };
static const QColor C_ACCENT  { "#FFD166" };
static const QColor C_CORAL   { "#FF6B6B" };
static const QColor C_MINT    { "#06D6A0" };
static const QColor C_TEXT    { "#FAFAFA" };
static const QColor C_MUTED   { "#A08BC0" };
static const QColor C_RING_BG { "#3B2265" };

static const QString GLOBAL_STYLE = R"(
QMainWindow, QWidget {
    background-color: #1A0A35;
    color: #FAFAFA;
    font-family: 'Nunito', 'Ubuntu', sans-serif;
}
QPushButton {
    background-color: #FFD166;
    color: #1A0A35;
    border: none;
    border-radius: 18px;
    padding: 12px 28px;
    font-size: 16px;
    font-weight: 700;
}
QPushButton:hover  { background-color: #ffe08a; }
QPushButton:pressed { background-color: #e6b800; }
QPushButton#stop {
    background-color: #FF6B6B;
    color: white;
    border-radius: 14px;
    padding: 8px 20px;
    font-size: 14px;
}
QPushButton#go {
    background-color: #06D6A0;
    color: #0a2e24;
    font-size: 20px;
    border-radius: 24px;
    padding: 16px 48px;
}
QMenuBar {
    background-color: #1A0A35;
    color: #A08BC0;
    font-size: 13px;
}
QMenuBar::item:selected { background: #2D1654; color: #FFD166; }
QMenu {
    background-color: #2D1654;
    color: #FAFAFA;
    border: 1px solid #3B2265;
    border-radius: 8px;
}
QMenu::item:selected { background: #3B2265; color: #FFD166; }
)";

// ── TimerRing ─────────────────────────────────────────────────────────────────
TimerRing::TimerRing(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(52, 52);
    setMaximumSize(52, 52);

    m_pulseAnim.setInterval(40);
    connect(&m_pulseAnim, &QTimer::timeout, this, [this]() {
        m_phase = (m_phase + 6) % 360;
        m_pulse = (1.0 + std::sin(m_phase * M_PI / 180.0)) / 2.0;
        update();
    });
}

void TimerRing::setTimes(int totalSec, int remainSec)
{
    m_total   = qMax(totalSec, 1);
    m_remain  = qMax(remainSec, 0);
    m_warning = remainSec <= 300;

    if (m_warning && !m_pulseAnim.isActive())
        m_pulseAnim.start();
    else if (!m_warning && m_pulseAnim.isActive()) {
        m_pulseAnim.stop();
        m_pulse = 0.0;
    }
    update();
}

void TimerRing::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int   sz  = qMin(width(), height());
    const qreal cx  = width()  / 2.0;
    const qreal cy  = height() / 2.0;
    const qreal R   = sz / 2.0 - 5;
    const qreal span = 360.0 * m_remain / m_total;

    // Background ring
    QPen bgPen(C_RING_BG, 5, Qt::SolidLine, Qt::RoundCap);
    p.setPen(bgPen);
    p.drawEllipse(QPointF(cx, cy), R, R);

    // Progress arc
    QColor arcColor;
    if (m_warning) {
        const QColor base("#FF6B6B"), bright("#ff9a9a");
        arcColor = QColor(
            base.red()   + int((bright.red()   - base.red())   * m_pulse),
            base.green() + int((bright.green() - base.green()) * m_pulse),
            base.blue()  + int((bright.blue()  - base.blue())  * m_pulse)
        );
    } else {
        arcColor = C_MINT;
    }

    QPen arcPen(arcColor, 5, Qt::SolidLine, Qt::RoundCap);
    p.setPen(arcPen);
    QRectF arcRect(cx - R, cy - R, R * 2, R * 2);
    p.drawArc(arcRect, 90 * 16, int(span * 16));
}

// ── MainWindow ────────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("🎬 Kids TV Timer");
    setMinimumSize(900, 640);
    setStyleSheet(GLOBAL_STYLE);

    // Central stacked widget
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    // Timer controller
    m_timer = new TimerController(this);

    // Build all screens
    buildSetupScreen();     // 0
    buildPlayerScreen();    // 1
    buildWaitScreen();      // 2
    buildCooldownScreen();  // 3
    buildLockedScreen();    // 4
    buildGoodbyeScreen();   // 5

    buildMenuBar();

    // Wire timer controller signals
    connect(m_timer, &TimerController::stateChanged,
            this, &MainWindow::onTimerStateChanged);
    connect(m_timer, &TimerController::sessionTick,
            this, &MainWindow::onSessionTick);
    connect(m_timer, &TimerController::cooldownTick,
            this, &MainWindow::onCooldownTick);
    connect(m_timer, &TimerController::waitTick,
            this, &MainWindow::onWaitTick);
    connect(m_timer, &TimerController::warning,
            this, &MainWindow::onWarning);
    connect(m_timer, &TimerController::sessionExpired,
            this, &MainWindow::onSessionExpired);
    connect(m_timer, &TimerController::videoShouldFinish,
            this, &MainWindow::onVideoShouldFinish);
    connect(m_timer, &TimerController::cooldownFinished,
            this, &MainWindow::onCooldownFinished);
    connect(m_timer, &TimerController::scheduleWindowOpened,
            this, &MainWindow::onScheduleWindowOpened);

    // Wire player signals
    connect(m_player, &PlayerWindow::videoRemainingUpdated,
            m_timer, &TimerController::updateVideoRemaining);
    connect(m_player, &PlayerWindow::blockedChannelDetected,
            this, &MainWindow::onBlockedChannel);

    showScreen(ScreenSetup);
}

// ── Menu bar ──────────────────────────────────────────────────────────────────
void MainWindow::buildMenuBar()
{
    auto* menu    = menuBar()->addMenu("⚙  Options");
    auto* pcAction = menu->addAction("🔐  Parental Controls…");
    pcAction->setShortcut(QKeySequence("Ctrl+P"));
    connect(pcAction, &QAction::triggered,
            this, &MainWindow::onOpenParentalControls);

    menu->addSeparator();
    auto* quitAction = menu->addAction("Quit");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
}

// ── Setup screen ──────────────────────────────────────────────────────────────
void MainWindow::buildSetupScreen()
{
    auto* screen = new QWidget();
    auto* lay    = new QVBoxLayout(screen);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    m_setupArtwork = new ArtworkFrame();
    m_setupArtwork->setMode(ArtworkFrame::Mode::Setup);
    lay->addWidget(m_setupArtwork);

    // Overlay — centered card on top of artwork
    auto* overlay = new QWidget(m_setupArtwork);
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    auto* ol = new QVBoxLayout(overlay);
    ol->setAlignment(Qt::AlignCenter);
    ol->setSpacing(20);

    auto* title = new QLabel("🎬  Kids TV");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "font-size: 36px; font-weight: bold; color: white;"
        "text-shadow: 2px 2px 8px rgba(0,0,0,0.8); background: transparent;"
    );
    ol->addWidget(title);

    m_setupInfo = new QLabel();
    m_setupInfo->setAlignment(Qt::AlignCenter);
    m_setupInfo->setStyleSheet(
        "font-size: 15px; color: rgba(255,255,255,0.8); background: transparent;"
    );
    m_setupInfo->setWordWrap(true);
    ol->addWidget(m_setupInfo);

    m_startBtn = new QPushButton("▶  Let's Watch!");
    m_startBtn->setObjectName("go");
    m_startBtn->setFixedWidth(260);
    connect(m_startBtn, &QPushButton::clicked,
            this, &MainWindow::onStartSession);
    ol->addWidget(m_startBtn, 0, Qt::AlignCenter);

    // Position overlay centered over artwork
    overlay->setGeometry(m_setupArtwork->rect());
    connect(m_setupArtwork, &ArtworkFrame::artworkChanged,
            this, [overlay, this](const QString&) {
        overlay->setGeometry(m_setupArtwork->rect());
    });

    // Update setup info from config
    auto updateInfo = [this]() {
        Config& cfg = Config::instance();
        m_setupInfo->setText(
            QString("⏱  %1 min watch  •  🎞  %2 min grace  •  ❄️  %3 min cooldown")
                .arg(cfg.sessionDuration()  / 60)
                .arg(cfg.gracePeriod()      / 60)
                .arg(cfg.cooldownDuration() / 60)
        );
    };
    updateInfo();
    connect(&Config::instance(), &Config::configChanged, this, updateInfo);

    m_stack->addWidget(screen);
}

// ── Player screen ─────────────────────────────────────────────────────────────
void MainWindow::buildPlayerScreen()
{
    auto* screen = new QWidget();
    auto* lay    = new QVBoxLayout(screen);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // Top bar
    buildTopBar();
    lay->addWidget(m_topBar);

    // Player
    m_player = new PlayerWindow();
    lay->addWidget(m_player);

    m_stack->addWidget(screen);
}

void MainWindow::buildTopBar()
{
    m_topBar = new QWidget();
    m_topBar->setFixedHeight(64);
    m_topBar->setStyleSheet("background: #2D1654;");

    auto* lay = new QHBoxLayout(m_topBar);
    lay->setContentsMargins(16, 8, 16, 8);
    lay->setSpacing(12);

    m_ring = new TimerRing();
    lay->addWidget(m_ring);

    m_timeLabel = new QLabel("00:00");
    m_timeLabel->setStyleSheet(
        "color: #FAFAFA; font-size: 20px; font-weight: bold; background: transparent;"
    );
    lay->addWidget(m_timeLabel);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet(
        "color: #A08BC0; font-size: 12px; background: transparent;"
    );
    lay->addWidget(m_statusLabel);

    lay->addStretch();

    m_stopBtn = new QPushButton("⏹  Stop");
    m_stopBtn->setObjectName("stop");
    connect(m_stopBtn, &QPushButton::clicked,
            this, &MainWindow::onStopSession);
    lay->addWidget(m_stopBtn);
}

// ── Wait screen ───────────────────────────────────────────────────────────────
void MainWindow::buildWaitScreen()
{
    auto* screen = new QWidget();
    auto* lay    = new QVBoxLayout(screen);
    lay->setContentsMargins(0, 0, 0, 0);

    m_waitArtwork = new ArtworkFrame();
    m_waitArtwork->setMode(ArtworkFrame::Mode::Waiting);
    m_waitArtwork->setCountdownText("🕐  Not yet…");
    m_waitArtwork->setStatusText("TV time starts soon!");
    lay->addWidget(m_waitArtwork);

    m_stack->addWidget(screen);
}

// ── Cooldown screen ───────────────────────────────────────────────────────────
void MainWindow::buildCooldownScreen()
{
    auto* screen = new QWidget();
    auto* lay    = new QVBoxLayout(screen);
    lay->setContentsMargins(0, 0, 0, 0);

    m_cooldownArtwork = new ArtworkFrame();
    m_cooldownArtwork->setMode(ArtworkFrame::Mode::Cooldown);
    m_cooldownArtwork->setCountdownText("❄️  Cool down time!");
    m_cooldownArtwork->setStatusText("Screen break — back soon 🌟");
    lay->addWidget(m_cooldownArtwork);

    m_stack->addWidget(screen);
}

// ── Locked screen ─────────────────────────────────────────────────────────────
void MainWindow::buildLockedScreen()
{
    auto* screen = new QWidget();
    auto* lay    = new QVBoxLayout(screen);
    lay->setContentsMargins(0, 0, 0, 0);

    m_lockedArtwork = new ArtworkFrame();
    m_lockedArtwork->setMode(ArtworkFrame::Mode::Locked);
    m_lockedArtwork->setCountdownText("🌙  See you later!");
    m_lockedArtwork->setStatusText("TV time is over for today");
    lay->addWidget(m_lockedArtwork);

    m_stack->addWidget(screen);
}

// ── Goodbye screen ────────────────────────────────────────────────────────────
void MainWindow::buildGoodbyeScreen()
{
    auto* screen = new QWidget();
    auto* lay    = new QVBoxLayout(screen);
    lay->setAlignment(Qt::AlignCenter);
    lay->setSpacing(24);

    auto* emoji = new QLabel("🌟");
    emoji->setAlignment(Qt::AlignCenter);
    emoji->setStyleSheet("font-size: 72px; background: transparent;");
    lay->addWidget(emoji);

    auto* title = new QLabel("Great job today!");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "font-size: 28px; font-weight: bold; color: #FAFAFA; background: transparent;"
    );
    lay->addWidget(title);

    auto* sub = new QLabel("Screen time is all done.\nSee you next time! 🎉");
    sub->setAlignment(Qt::AlignCenter);
    sub->setStyleSheet(
        "font-size: 16px; color: #A08BC0; background: transparent;"
    );
    lay->addWidget(sub);

    auto* btn = new QPushButton("Set up again");
    btn->setFixedWidth(200);
    connect(btn, &QPushButton::clicked, this, [this]() {
        showScreen(ScreenSetup);
    });
    lay->addWidget(btn, 0, Qt::AlignCenter);

    m_stack->addWidget(screen);
}

// ── Screen switching ──────────────────────────────────────────────────────────
void MainWindow::showScreen(int index)
{
    m_stack->setCurrentIndex(index);
}

// ── Top bar update ────────────────────────────────────────────────────────────
void MainWindow::updateTopBar(int remainSec, int totalSec)
{
    m_ring->setTimes(totalSec, remainSec);
    m_timeLabel->setText(formatTime(remainSec));

    if (remainSec <= 60)
        m_timeLabel->setStyleSheet(
            "color: #FF6B6B; font-size: 20px; font-weight: bold; background: transparent;"
        );
    else if (remainSec <= 300)
        m_timeLabel->setStyleSheet(
            "color: #FFD166; font-size: 20px; font-weight: bold; background: transparent;"
        );
    else
        m_timeLabel->setStyleSheet(
            "color: #FAFAFA; font-size: 20px; font-weight: bold; background: transparent;"
        );
}

// ── Timer slots ───────────────────────────────────────────────────────────────
void MainWindow::onTimerStateChanged(TimerController::State state)
{
    using S = TimerController::State;
    switch (state) {
    case S::Running:
    case S::Warning5:
    case S::Warning1:
        showScreen(ScreenPlayer);
        m_player->startPolling();
        break;
    case S::GraceFinish:
        m_statusLabel->setText("🎬 Finishing episode…");
        break;
    case S::Waiting:
        showScreen(ScreenWait);
        break;
    case S::Cooldown:
        m_player->stopPolling();
        showScreen(ScreenCooldown);
        break;
    case S::Locked:
        showScreen(ScreenLocked);
        break;
    case S::Idle:
        break;
    }
}

void MainWindow::onSessionTick(int remaining)
{
    updateTopBar(remaining, m_timer->sessionRemaining() + 1);
}

void MainWindow::onCooldownTick(int remaining)
{
    m_cooldownArtwork->setStatusText(
        QString("❄️  Cool down: %1").arg(formatTime(remaining))
    );
}

void MainWindow::onWaitTick(int remaining)
{
    m_waitArtwork->setCountdownText(formatTime(remaining));
    m_waitArtwork->setStatusText(
        QString("TV time starts at %1")
            .arg(Config::instance().allowedStart().toString("HH:mm"))
    );
}

void MainWindow::onWarning(int minutesLeft)
{
    showWarningDialog(minutesLeft);
}

void MainWindow::onSessionExpired()
{
    m_player->onSessionExpired();
    showScreen(ScreenGoodbye);
}

void MainWindow::onVideoShouldFinish()
{
    m_player->onVideoShouldFinish();
    m_statusLabel->setText("🎬 Finishing episode…");
}

void MainWindow::onCooldownFinished()
{
    showScreen(ScreenGoodbye);
}

void MainWindow::onScheduleWindowOpened()
{
    // Timer auto-starts session — state change handles screen switch
}

// ── Session control ───────────────────────────────────────────────────────────
void MainWindow::onStartSession()
{
    m_player->loadHome();
    m_timer->startSession();
}

void MainWindow::onStopSession()
{
    auto reply = QMessageBox::question(
        this, "Stop watching?",
        "Are you sure you want to stop?",
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        m_timer->stopSession();
        m_player->onSessionExpired();
        showScreen(ScreenGoodbye);
    }
}

// ── Parental controls ─────────────────────────────────────────────────────────
void MainWindow::onOpenParentalControls()
{
    const bool wasPaused = m_timer->isPaused();
    if (m_timer->state() == TimerController::State::Running ||
        m_timer->state() == TimerController::State::Warning5 ||
        m_timer->state() == TimerController::State::Warning1) {
        m_timer->pauseSession();
        m_player->pauseVideo();
    }

    openParentalControls(this);

    if (!wasPaused)
        m_timer->resumeSession();

    // Re-apply channel filters if config changed
    m_player->applyChannelFilters();
}

// ── Blocked channel ───────────────────────────────────────────────────────────
void MainWindow::onBlockedChannel(const QString& name)
{
    Q_UNUSED(name)
    m_player->loadHome();
}

// ── Warning dialog ────────────────────────────────────────────────────────────
void MainWindow::showWarningDialog(int minutesLeft)
{
    auto* dlg = new QDialog(this, Qt::FramelessWindowHint);
    dlg->setModal(true);
    dlg->setFixedSize(380, 280);
    dlg->setStyleSheet(R"(
        QDialog {
            background: #1A0A35;
            border: 3px solid #FF6B6B;
            border-radius: 20px;
        }
        QLabel { background: transparent; color: white; }
        QPushButton {
            background: #FFD166;
            color: #1A0A35;
            border: none;
            border-radius: 14px;
            padding: 12px 32px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover { background: #ffe08a; }
    )");

    auto* lay = new QVBoxLayout(dlg);
    lay->setSpacing(16);
    lay->setContentsMargins(36, 32, 36, 32);

    auto* emoji = new QLabel(minutesLeft > 1 ? "⏰" : "🚨");
    emoji->setAlignment(Qt::AlignCenter);
    emoji->setStyleSheet("font-size: 52px; background: transparent;");
    lay->addWidget(emoji);

    auto* msg = new QLabel(
        QString("<b style='font-size:20px;'>%1 minute%2 left!</b><br>"
                "<span style='font-size:14px;color:#A08BC0;'>"
                "Time to wrap up soon 🎬</span>")
            .arg(minutesLeft)
            .arg(minutesLeft > 1 ? "s" : "")
    );
    msg->setAlignment(Qt::AlignCenter);
    msg->setTextFormat(Qt::RichText);
    lay->addWidget(msg);

    auto* btn = new QPushButton("Got it! 👍");
    connect(btn, &QPushButton::clicked, dlg, &QDialog::accept);
    lay->addWidget(btn, 0, Qt::AlignCenter);

    dlg->exec();
    dlg->deleteLater();
}

// ── Key events ────────────────────────────────────────────────────────────────
void MainWindow::keyPressEvent(QKeyEvent* event)
{
    // Ctrl+P anywhere opens parental controls
    if (event->modifiers() == Qt::ControlModifier &&
        event->key() == Qt::Key_P) {
        onOpenParentalControls();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

// ── Close ─────────────────────────────────────────────────────────────────────
void MainWindow::closeEvent(QCloseEvent* event)
{
    m_timer->stopSession();
    m_player->stopPolling();
    event->accept();
}

// ── Helpers ───────────────────────────────────────────────────────────────────
QString MainWindow::formatTime(int seconds) const
{
    const int m = seconds / 60;
    const int s = seconds % 60;
    return QString("%1:%2")
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

} // namespace YTKids
