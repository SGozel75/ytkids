#include "artworkframe.h"
#include "config.h"

#include <QPainter>
#include <QPainterPath>
#include <QFileDialog>
#include <QDir>
#include <QImageReader>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QStandardPaths>

namespace YTKids {

// Supported image formats
static const QStringList IMAGE_FILTERS = {
    "*.jpg", "*.jpeg", "*.png", "*.bmp", "*.gif", "*.webp"
};

ArtworkFrame::ArtworkFrame(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);

    buildOverlay();

    // Slideshow — cycles artwork every 30s on waiting/locked screens
    m_slideshowTimer.setInterval(30000);
    m_slideshowTimer.setSingleShot(false);
    connect(&m_slideshowTimer, &QTimer::timeout,
            this, &ArtworkFrame::onSlideshow);

    // Load saved artwork
    const QString saved = Config::instance().artworkPath();
    if (!saved.isEmpty() && QFile::exists(saved))
        loadArtwork(saved);
    else
        loadDefaultArtwork();
}

// ── Build overlay widgets ─────────────────────────────────────────────────────
void ArtworkFrame::buildOverlay()
{
    // Countdown label — centered, large
    m_countdownLabel = new QLabel(this);
    m_countdownLabel->setAlignment(Qt::AlignCenter);
    m_countdownLabel->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 42px;
            font-weight: bold;
            background: transparent;
            text-shadow: 2px 2px 8px rgba(0,0,0,0.8);
        }
    )");
    m_countdownLabel->hide();

    // Status label — below countdown
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(R"(
        QLabel {
            color: rgba(255,255,255,0.85);
            font-size: 16px;
            background: transparent;
            text-shadow: 1px 1px 4px rgba(0,0,0,0.8);
        }
    )");
    m_statusLabel->hide();

    // Change picture button — bottom right, hidden by default
    m_changeBtn = new QPushButton("🖼  Change picture", this);
    m_changeBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(255, 209, 102, 0.85);
            color: #1A0A35;
            border: none;
            border-radius: 14px;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover { background: rgba(255, 224, 138, 0.95); }
        QPushButton:pressed { background: rgba(230, 184, 0, 0.95); }
    )");
    m_changeBtn->hide();
    m_changeBtn->raise();
    connect(m_changeBtn, &QPushButton::clicked,
            this, &ArtworkFrame::onChangePicture);

    resizeEvent(nullptr);   // position widgets
}

// ── Artwork loading ───────────────────────────────────────────────────────────
void ArtworkFrame::loadArtwork(const QString& path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);

    QImage img = reader.read();
    if (img.isNull()) return;

    m_pixmap = QPixmap::fromImage(img);
    Config::instance().setArtworkPath(path);
    update();
    emit artworkChanged(path);

    // Refresh slideshow list from same folder
    m_folderImages = imagesInFolder();
    m_slideshowIndex = m_folderImages.indexOf(path);
}

void ArtworkFrame::loadDefaultArtwork()
{
    // Try to find any image in the configured artwork folder
    m_folderImages = imagesInFolder();
    if (!m_folderImages.isEmpty()) {
        loadArtwork(m_folderImages.first());
        return;
    }

    // Absolute fallback — solid gradient, drawn in paintEvent
    m_pixmap = QPixmap();
    update();
}

QStringList ArtworkFrame::imagesInFolder() const
{
    const QString folder = Config::instance().artworkFolder();
    if (folder.isEmpty()) return {};

    QDir dir(folder);
    QStringList files;
    for (const QString& f : dir.entryList(IMAGE_FILTERS, QDir::Files, QDir::Name))
        files << dir.filePath(f);
    return files;
}

// ── Mode ──────────────────────────────────────────────────────────────────────
void ArtworkFrame::setMode(Mode mode)
{
    m_mode = mode;

    switch (mode) {
    case Mode::Setup:
        m_countdownLabel->hide();
        m_statusLabel->hide();
        m_slideshowTimer.stop();
        break;

    case Mode::Waiting:
        m_countdownLabel->show();
        m_statusLabel->show();
        m_folderImages = imagesInFolder();
        if (m_folderImages.size() > 1)
            m_slideshowTimer.start();
        break;

    case Mode::Cooldown:
        m_countdownLabel->show();
        m_statusLabel->show();
        m_changeBtn->hide();
        m_btnVisible = false;
        m_slideshowTimer.stop();
        break;

    case Mode::Locked:
        m_countdownLabel->show();
        m_statusLabel->show();
        m_changeBtn->hide();
        m_btnVisible = false;
        m_folderImages = imagesInFolder();
        if (m_folderImages.size() > 1)
            m_slideshowTimer.start();
        break;
    }

    update();
}

void ArtworkFrame::setCountdownText(const QString& text)
{
    m_countdownLabel->setText(text);
}

void ArtworkFrame::setStatusText(const QString& text)
{
    m_statusLabel->setText(text);
}

// ── Paint ─────────────────────────────────────────────────────────────────────
void ArtworkFrame::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    const QRect r = rect();

    if (!m_pixmap.isNull()) {
        // Scale to fill, centered (cover behavior)
        const QPixmap scaled = m_pixmap.scaled(
            r.size(),
            Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation
        );
        const QPoint offset(
            (r.width()  - scaled.width())  / 2,
            (r.height() - scaled.height()) / 2
        );
        p.drawPixmap(offset, scaled);
    } else {
        // Fallback gradient
        QLinearGradient grad(0, 0, r.width(), r.height());
        grad.setColorAt(0, QColor("#1A0A35"));
        grad.setColorAt(1, QColor("#3B2265"));
        p.fillRect(r, grad);
    }

    // Dark overlay for readability of text labels
    if (m_mode != Mode::Setup || m_countdownLabel->isVisible()) {
        p.fillRect(r, QColor(0, 0, 0,
                   static_cast<int>(m_overlayOpacity * 255)));
    }
}

// ── Resize — reposition overlay widgets ──────────────────────────────────────
void ArtworkFrame::resizeEvent(QResizeEvent*)
{
    const int w = width();
    const int h = height();

    if (m_countdownLabel) {
        m_countdownLabel->setGeometry(0, h / 2 - 80, w, 60);
    }
    if (m_statusLabel) {
        m_statusLabel->setGeometry(0, h / 2 - 10, w, 30);
    }
    if (m_changeBtn) {
        const int bw = 180, bh = 44;
        m_changeBtn->setGeometry(w - bw - 16, h - bh - 16, bw, bh);
    }
}

// ── Hover — show/hide change button ──────────────────────────────────────────
void ArtworkFrame::enterEvent(QEnterEvent*)
{
    if (m_mode == Mode::Setup || m_mode == Mode::Waiting)
        animateButtonIn();
}

void ArtworkFrame::leaveEvent(QEvent*)
{
    animateButtonOut();
}

void ArtworkFrame::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
}

void ArtworkFrame::animateButtonIn()
{
    if (m_btnVisible) return;
    m_btnVisible = true;
    m_changeBtn->show();

    auto* fx  = new QGraphicsOpacityEffect(m_changeBtn);
    m_changeBtn->setGraphicsEffect(fx);

    auto* anim = new QPropertyAnimation(fx, "opacity", this);
    anim->setDuration(200);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void ArtworkFrame::animateButtonOut()
{
    if (!m_btnVisible) return;
    m_btnVisible = false;

    auto* fx = qobject_cast<QGraphicsOpacityEffect*>(
        m_changeBtn->graphicsEffect());
    if (!fx) {
        m_changeBtn->hide();
        return;
    }

    auto* anim = new QPropertyAnimation(fx, "opacity", this);
    anim->setDuration(200);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    connect(anim, &QPropertyAnimation::finished,
            m_changeBtn, &QWidget::hide);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// ── Change picture ────────────────────────────────────────────────────────────
void ArtworkFrame::onChangePicture()
{
    const QString folder = Config::instance().artworkFolder();
    const QString start  = folder.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
        : folder;

    const QString path = QFileDialog::getOpenFileName(
        this,
        "Choose your picture 🎨",
        start,
        "Images (*.jpg *.jpeg *.png *.bmp *.gif *.webp)"
    );

    if (!path.isEmpty())
        loadArtwork(path);
}

// ── Slideshow ─────────────────────────────────────────────────────────────────
void ArtworkFrame::onSlideshow()
{
    if (m_folderImages.isEmpty()) return;

    m_slideshowIndex = (m_slideshowIndex + 1) % m_folderImages.size();
    loadArtwork(m_folderImages.at(m_slideshowIndex));
}

} // namespace YTKids
