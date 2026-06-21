#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QSpinBox>
#include <QTimeEdit>
#include <QCheckBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

namespace YTKids {

// ── PIN Entry Dialog ──────────────────────────────────────────────────────────
// Shown before opening parental settings
class PinDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PinDialog(QWidget* parent = nullptr);
    bool verified() const { return m_verified; }

private slots:
    void onAccept();
    void onDigit(const QString& digit);
    void onBackspace();

private:
    void buildNumpad();
    void updateDots();
    void shake();           // wrong PIN animation

    QLabel*      m_dots    { nullptr };
    QString      m_input;
    bool         m_verified { false };
};

// ── Parental Control Panel ────────────────────────────────────────────────────
class ParentalControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ParentalControlDialog(QWidget* parent = nullptr);

private slots:
    void onSave();
    void onReset();
    void onChangePin();
    void onAddAllowedChannel();
    void onRemoveAllowedChannel();
    void onAddBlockedChannel();
    void onRemoveBlockedChannel();
    void onBrowseArtworkFolder();

private:
    void buildSessionTab(QWidget* tab);
    void buildScheduleTab(QWidget* tab);
    void buildContentTab(QWidget* tab);
    void buildArtworkTab(QWidget* tab);
    void buildSecurityTab(QWidget* tab);
    void loadFromConfig();
    void saveToConfig();

    // Session tab
    QSpinBox*   m_sessionSpin   { nullptr };    // minutes
    QSpinBox*   m_graceSpin     { nullptr };    // minutes
    QSpinBox*   m_cooldownSpin  { nullptr };    // minutes

    // Schedule tab
    QCheckBox*  m_scheduleCheck { nullptr };
    QTimeEdit*  m_startTime     { nullptr };
    QTimeEdit*  m_endTime       { nullptr };

    // Content tab
    QListWidget* m_allowedChannels { nullptr };
    QListWidget* m_blockedChannels { nullptr };
    QLineEdit*   m_channelInput    { nullptr };

    // Artwork tab
    QLineEdit*   m_artworkFolder   { nullptr };

    // Security tab
    QLineEdit*   m_newPin          { nullptr };
    QLineEdit*   m_confirmPin      { nullptr };

    QTabWidget*  m_tabs            { nullptr };
};

// ── Gate function — use this everywhere ──────────────────────────────────────
// Returns true if user passed PIN check (or no PIN set)
// and opens the parental dialog on success
bool openParentalControls(QWidget* parent = nullptr);

} // namespace YTKids
