#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QStringList>
#include <QTimer>

namespace YTKids {

// ── ArtworkFrame ──────────────────────────────────────────────────────────────
// Displays the child's chosen artwork as a full-bleed background.
// Shows a subtle "change picture" button on hover.
// Used on: setup screen, waiting screen, cooldown screen, locked screen.

class ArtworkFrame : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal overlayOpacity READ overlayOpacity
                                    WRITE setOverlayOpacity)

public:
    explicit ArtworkFrame(QWidget* parent = nullptr);

    // ── Display modes ─────────────────────────────────────────────────────────
    enum class Mode {
        Setup,      // show change button, session info overlay
        Waiting,    // countdown to allowed time, change button
        Cooldown,   // cooldown countdown, no change button
        Locked      // locked screen, no change button
    };

    void setMode(Mode mode);
    void setCountdownText(const QString& text);     // "Available in 14:32"
    void setStatusText(const QString& text);        // "Cool down: 08:00"

    // ── Artwork ───────────────────────────────────────────────────────────────
    void loadArtwork(const QString& path);
    void loadDefaultArtwork();
    bool hasArtwork() const { return !m_pixmap.isNull(); }

    qreal overlayOpacity() const        { return m_overlayOpacity; }
    void  setOverlayOpacity(qreal v)    { m_overlayOpacity = v; update(); }

signals:
    void artworkChanged(const QString& newPath);

protected:
    void paintEvent(QPaintEvent* event)         override;
    void resizeEvent(QResizeEvent* event)       override;
    void enterEvent(QEnterEvent* event)         override;
    void leaveEvent(QEvent* event)              override;
    void mousePressEvent(QMouseEvent* event)    override;

private slots:
    void onChangePicture();
    void onSlideshow();         // cycles through folder on waiting/locked screens

private:
    void buildOverlay();
    void animateButtonIn();
    void animateButtonOut();
    QStringList imagesInFolder() const;

    Mode        m_mode              { Mode::Setup };
    QPixmap     m_pixmap;
    qreal       m_overlayOpacity    { 0.45 };

    QLabel*     m_countdownLabel    { nullptr };
    QLabel*     m_statusLabel       { nullptr };
    QPushButton* m_changeBtn        { nullptr };

    QTimer      m_slideshowTimer;
    QStringList m_folderImages;
    int         m_slideshowIndex    { 0 };

    bool        m_btnVisible        { false };
};

} // namespace YTKids
