#include "config.h"

#include <QSettings>
#include <QCryptographicHash>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>

namespace YTKids {

// ── Salt — change before release ──────────────────────────────────────────────
static const QString PIN_SALT = QStringLiteral("ytkids::v1::pinSalt");

// ── Defaults ──────────────────────────────────────────────────────────────────
namespace Defaults {
    static constexpr int SESSION_DURATION  = 1800;  // 30 min
    static constexpr int GRACE_PERIOD      = 300;   // 5 min
    static constexpr int COOLDOWN_DURATION = 1800;  // 30 min
    static const QTime   ALLOWED_START     = QTime(15, 0);
    static const QTime   ALLOWED_END       = QTime(19, 0);
    static const bool    SCHEDULE_ENABLED  = false;
}

// ── Singleton ─────────────────────────────────────────────────────────────────
Config& Config::instance()
{
    static Config inst;
    return inst;
}

Config::Config(QObject* parent)
    : QObject(parent)
{
    QSettings::setDefaultFormat(QSettings::IniFormat);
}

// ── Session ───────────────────────────────────────────────────────────────────
int Config::sessionDuration() const
{
    QSettings s;
    return s.value("session/duration", Defaults::SESSION_DURATION).toInt();
}

void Config::setSessionDuration(int seconds)
{
    QSettings s;
    s.setValue("session/duration", seconds);
    emit configChanged();
}

int Config::gracePeriod() const
{
    QSettings s;
    return s.value("session/gracePeriod", Defaults::GRACE_PERIOD).toInt();
}

void Config::setGracePeriod(int seconds)
{
    QSettings s;
    s.setValue("session/gracePeriod", seconds);
    emit configChanged();
}

int Config::cooldownDuration() const
{
    QSettings s;
    return s.value("session/cooldown", Defaults::COOLDOWN_DURATION).toInt();
}

void Config::setCooldownDuration(int seconds)
{
    QSettings s;
    s.setValue("session/cooldown", seconds);
    emit configChanged();
}

// ── Schedule ──────────────────────────────────────────────────────────────────
QTime Config::allowedStart() const
{
    QSettings s;
    return s.value("schedule/start", Defaults::ALLOWED_START).toTime();
}

void Config::setAllowedStart(const QTime& time)
{
    QSettings s;
    s.setValue("schedule/start", time);
    emit configChanged();
}

QTime Config::allowedEnd() const
{
    QSettings s;
    return s.value("schedule/end", Defaults::ALLOWED_END).toTime();
}

void Config::setAllowedEnd(const QTime& time)
{
    QSettings s;
    s.setValue("schedule/end", time);
    emit configChanged();
}

bool Config::scheduleEnabled() const
{
    QSettings s;
    return s.value("schedule/enabled", Defaults::SCHEDULE_ENABLED).toBool();
}

void Config::setScheduleEnabled(bool enabled)
{
    QSettings s;
    s.setValue("schedule/enabled", enabled);
    emit configChanged();
}

bool Config::isWithinAllowedWindow() const
{
    if (!scheduleEnabled())
        return true;

    const QTime now   = QTime::currentTime();
    const QTime start = allowedStart();
    const QTime end   = allowedEnd();

    // Handles overnight windows (e.g. 22:00 → 06:00) gracefully
    if (start <= end)
        return now >= start && now < end;
    else
        return now >= start || now < end;
}

// ── Parental PIN ──────────────────────────────────────────────────────────────
bool Config::hasPin() const
{
    QSettings s;
    return s.contains("parental/pinHash");
}

bool Config::verifyPin(const QString& pin) const
{
    QSettings s;
    const QString stored = s.value("parental/pinHash").toString();
    return !stored.isEmpty() && stored == hashPin(pin);
}

void Config::setPin(const QString& pin)
{
    QSettings s;
    s.setValue("parental/pinHash", hashPin(pin));
    emit configChanged();
}

void Config::clearPin()
{
    QSettings s;
    s.remove("parental/pinHash");
    emit configChanged();
}

QString Config::hashPin(const QString& pin) const
{
    const QByteArray salted = (PIN_SALT + pin).toUtf8();
    return QString::fromLatin1(
        QCryptographicHash::hash(salted, QCryptographicHash::Sha256).toHex()
    );
}

// ── Artwork ───────────────────────────────────────────────────────────────────
QString Config::artworkPath() const
{
    QSettings s;
    return s.value("artwork/current", QString()).toString();
}

void Config::setArtworkPath(const QString& path)
{
    QSettings s;
    s.setValue("artwork/current", path);
    emit configChanged();
}

QString Config::artworkFolder() const
{
    QSettings s;
    const QString defaultFolder =
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    return s.value("artwork/folder", defaultFolder).toString();
}

void Config::setArtworkFolder(const QString& path)
{
    QSettings s;
    s.setValue("artwork/folder", path);
    emit configChanged();
}

// ── Content — Keywords ────────────────────────────────────────────────────────
QStringList Config::allowedKeywords() const
{
    QSettings s;
    return s.value("content/allowedKeywords").toStringList();
}

void Config::setAllowedKeywords(const QStringList& keywords)
{
    QSettings s;
    s.setValue("content/allowedKeywords", keywords);
    emit configChanged();
}

QStringList Config::blockedKeywords() const
{
    QSettings s;
    return s.value("content/blockedKeywords").toStringList();
}

void Config::setBlockedKeywords(const QStringList& keywords)
{
    QSettings s;
    s.setValue("content/blockedKeywords", keywords);
    emit configChanged();
}

// ── Content — Channels ────────────────────────────────────────────────────────
// Stored as "DisplayName|ChannelID" pairs, e.g. "Blippi|UCnd7djBa3RL8K9hFp_wqBhg"

QStringList Config::allowedChannels() const
{
    QSettings s;
    return s.value("content/allowedChannels").toStringList();
}

void Config::setAllowedChannels(const QStringList& channels)
{
    QSettings s;
    s.setValue("content/allowedChannels", channels);
    emit configChanged();
}

QStringList Config::blockedChannels() const
{
    QSettings s;
    return s.value("content/blockedChannels").toStringList();
}

void Config::setBlockedChannels(const QStringList& channels)
{
    QSettings s;
    s.setValue("content/blockedChannels", channels);
    emit configChanged();
}

// ── Reset ─────────────────────────────────────────────────────────────────────
void Config::resetToDefaults()
{
    QSettings s;
    s.clear();
    setSessionDuration(Defaults::SESSION_DURATION);
    setGracePeriod(Defaults::GRACE_PERIOD);
    setCooldownDuration(Defaults::COOLDOWN_DURATION);
    setAllowedStart(Defaults::ALLOWED_START);
    setAllowedEnd(Defaults::ALLOWED_END);
    setScheduleEnabled(Defaults::SCHEDULE_ENABLED);
    emit configChanged();
}

} // namespace YTKids
