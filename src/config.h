#pragma once

#include <QObject>
#include <QString>
#include <QTime>
#include <QStringList>

namespace YTKids {

class Config : public QObject
{
    Q_OBJECT

public:
    static Config& instance();

    // ── Session ───────────────────────────────────────────────────────────────
    int     sessionDuration() const;    // seconds
    void    setSessionDuration(int seconds);

    int     gracePeriod() const;        // seconds — finish video if under this
    void    setGracePeriod(int seconds);

    int     cooldownDuration() const;   // seconds — no-screen time after session
    void    setCooldownDuration(int seconds);

    // ── Schedule ──────────────────────────────────────────────────────────────
    QTime   allowedStart() const;
    void    setAllowedStart(const QTime& time);

    QTime   allowedEnd() const;
    void    setAllowedEnd(const QTime& time);

    bool    scheduleEnabled() const;
    void    setScheduleEnabled(bool enabled);

    // ── Parental PIN ──────────────────────────────────────────────────────────
    bool    hasPin() const;
    bool    verifyPin(const QString& pin) const;
    void    setPin(const QString& pin);         // stores bcrypt-style hash
    void    clearPin();

    // ── Artwork ───────────────────────────────────────────────────────────────
    QString artworkPath() const;                // current selected image
    void    setArtworkPath(const QString& path);

    QString artworkFolder() const;              // parent-approved source folder
    void    setArtworkFolder(const QString& path);

    // ── Content preferences ───────────────────────────────────────────────────
    QStringList allowedKeywords() const;
    void        setAllowedKeywords(const QStringList& keywords);

    QStringList blockedKeywords() const;
    void        setBlockedKeywords(const QStringList& keywords);
// ── Content — Channels ────────────────────────────────────────────────
    // Format: "DisplayName|ChannelID"
    QStringList allowedChannels() const;
    void        setAllowedChannels(const QStringList& channels);

    QStringList blockedChannels() const;
    void        setBlockedChannels(const QStringList& channels);
    // ── Utility ───────────────────────────────────────────────────────────────
    void    resetToDefaults();
    bool    isWithinAllowedWindow() const;      // checks current time vs schedule

signals:
    void configChanged();
    
private:
    explicit Config(QObject* parent = nullptr);
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    QString hashPin(const QString& pin) const;
};

} // namespace YTKids
