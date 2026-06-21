#include <QStandardPaths>
#include "playerwindow.h"
#include "config.h"

#include <QVBoxLayout>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineSettings>
#include <QApplication>

namespace YTKids {

// ── JS bridge injected into every page ───────────────────────────────────────
const char* PlayerWindow::JS_BRIDGE = R"js(
(function() {
    'use strict';

    // Video remaining time
    window._ytk_getVideoRemaining = function() {
        var v = document.querySelector('video');
        if (!v || isNaN(v.duration)) return -1;
        return Math.max(0, Math.round(v.duration - v.currentTime));
    };

    // Pause current video
    window._ytk_pause = function() {
        var v = document.querySelector('video');
        if (v) v.pause();
    };

    // Current channel ID from URL
    window._ytk_getChannelId = function() {
        var m = window.location.href.match(/\/channel\/(UC[A-Za-z0-9_-]+)/);
        return m ? m[1] : '';
    };

    // Current channel name from page
    window._ytk_getChannelName = function() {
        var el = document.querySelector('yt-formatted-string.ytd-channel-name');
        return el ? el.textContent.trim() : '';
    };

})();
)js";

// ── PlayerPage ────────────────────────────────────────────────────────────────
PlayerPage::PlayerPage(QWebEngineProfile* profile, QObject* parent)
    : QWebEnginePage(profile, parent)
{}

bool PlayerPage::acceptNavigationRequest(const QUrl& url,
                                          NavigationType type,
                                          bool isMainFrame)
{
    Q_UNUSED(type)
    Q_UNUSED(isMainFrame)

    if (isBlockedUrl(url)) {
        emit blockedChannelDetected(url.toString());
        return false;   // block navigation
    }
    return true;
}

bool PlayerPage::isBlockedUrl(const QUrl& url) const
{
    const QString urlStr = url.toString().toLower();
    const QStringList blocked = Config::instance().blockedChannels();

    for (const QString& entry : blocked) {
        // entry format: "DisplayName|ChannelID"
        const QStringList parts = entry.split('|');
        if (parts.size() == 2) {
            const QString id = parts.at(1).trimmed().toLower();
            if (!id.isEmpty() && urlStr.contains(id))
                return true;
        }
        // fallback — match by display name substring
        const QString name = parts.first().trimmed().toLower();
        if (!name.isEmpty() && urlStr.contains(name))
            return true;
    }
    return false;
}

// ── PlayerWindow ──────────────────────────────────────────────────────────────
PlayerWindow::PlayerWindow(QWidget* parent)
    : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    setupProfile();

    m_page = new PlayerPage(m_profile, this);
    m_view = new QWebEngineView(this);
    m_view->setPage(m_page);

    // WebEngine settings
    auto* settings = m_page->settings();
    settings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);

    lay->addWidget(m_view);

    // Poll timer — every 2 seconds
    m_pollTimer.setInterval(2000);
    m_pollTimer.setSingleShot(false);
    connect(&m_pollTimer, &QTimer::timeout,
            this, &PlayerWindow::onPollTimer);

    connect(m_page, &QWebEnginePage::loadFinished,
            this, &PlayerWindow::onPageLoadFinished);

    connect(m_page, &PlayerPage::blockedChannelDetected,
            this, &PlayerWindow::blockedChannelDetected);

    loadHome();
}

PlayerWindow::~PlayerWindow()
{
    stopPolling();
}

// ── Profile setup ─────────────────────────────────────────────────────────────
void PlayerWindow::setupProfile()
{
    // Named persistent profile — keeps YT Kids login between sessions
    m_profile = new QWebEngineProfile("ytkids_profile", this);
    m_profile->setHttpUserAgent(
        "Mozilla/5.0 (X11; Linux x86_64) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/120.0.0.0 Safari/537.36"
    );

    // Cache location
    m_profile->setCachePath(
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + "/ytkids_browser"
    );
    m_profile->setPersistentStoragePath(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/ytkids_browser"
    );
}

// ── Navigation ────────────────────────────────────────────────────────────────
void PlayerWindow::loadHome()
{
    m_view->load(QUrl("https://www.youtube.com/kids/"));
}

// ── JS injection ──────────────────────────────────────────────────────────────
void PlayerWindow::injectBridgeScript()
{
    QWebEngineScript script;
    script.setName("ytkids_bridge");
    script.setSourceCode(QString::fromLatin1(JS_BRIDGE));
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setRunsOnSubFrames(false);

    // Remove old version first
const auto existing = m_page->scripts().find("ytkids_bridge");
for (const auto& s : existing)
    m_page->scripts().remove(s);

    // Also run immediately on current page
    m_page->runJavaScript(QString::fromLatin1(JS_BRIDGE));
}

void PlayerWindow::injectFilterScript()
{
    const QString script = buildFilterScript();
    if (!script.isEmpty())
        m_page->runJavaScript(script);
}

QString PlayerWindow::buildFilterScript() const
{
    const QStringList blocked = Config::instance().blockedChannels();
    if (blocked.isEmpty()) return {};

    QStringList ids, names;
    for (const QString& entry : blocked) {
        const QStringList parts = entry.split('|');
        if (parts.size() == 2 && !parts.at(1).trimmed().isEmpty())
            ids   << QString("'%1'").arg(parts.at(1).trimmed());
        if (!parts.first().trimmed().isEmpty())
            names << QString("'%1'").arg(parts.first().trimmed().toLower());
    }

    return QString(R"js(
(function() {
    var blockedIds   = [%1];
    var blockedNames = [%2];

    function shouldHide(el) {
        var text = el ? el.innerText.toLowerCase() : '';
        for (var n of blockedNames) {
            if (text.includes(n)) return true;
        }
        return false;
    }

    function filterCards() {
        var cards = document.querySelectorAll(
            'ytd-rich-item-renderer, ytd-video-renderer, ytd-grid-video-renderer'
        );
        cards.forEach(function(card) {
            if (shouldHide(card)) {
                card.style.display = 'none';
            }
        });
    }

    filterCards();

    var observer = new MutationObserver(filterCards);
    observer.observe(document.body, { childList: true, subtree: true });
})();
)js").arg(ids.join(','), names.join(','));
}

// ── Polling ───────────────────────────────────────────────────────────────────
void PlayerWindow::startPolling()
{
    m_polling   = true;
    m_finishing = false;
    m_pollTimer.start();
}

void PlayerWindow::stopPolling()
{
    m_polling = false;
    m_pollTimer.stop();
}

void PlayerWindow::onPollTimer()
{
    m_page->runJavaScript(
        "window._ytk_getVideoRemaining ? window._ytk_getVideoRemaining() : -1;",
        [this](const QVariant& result) {
            onVideoRemainingResult(result);
        }
    );
}

void PlayerWindow::onVideoRemainingResult(const QVariant& result)
{
    const int secs = result.isValid() ? result.toInt() : -1;
    emit videoRemainingUpdated(secs);

    // If in grace/finish mode and video just ended — signal done
    if (m_finishing && secs >= 0 && secs < 2) {
        stopPolling();
        onSessionExpired();
    }
}

// ── Session control ───────────────────────────────────────────────────────────
void PlayerWindow::onSessionExpired()
{
    stopPolling();
    m_page->runJavaScript(
        "if(window._ytk_pause) window._ytk_pause();"
    );
    m_view->load(QUrl("about:blank"));
}

void PlayerWindow::onVideoShouldFinish()
{
    m_finishing = true;
    // polling continues — onVideoRemainingResult handles the stop
}

void PlayerWindow::pauseVideo()
{
    m_page->runJavaScript(
        "if(window._ytk_pause) window._ytk_pause();"
    );
}

// ── Channel filters ───────────────────────────────────────────────────────────
void PlayerWindow::applyChannelFilters()
{
    injectFilterScript();
}

// ── Page load ─────────────────────────────────────────────────────────────────
void PlayerWindow::onPageLoadFinished(bool ok)
{
    if (!ok) return;
    injectBridgeScript();
    injectFilterScript();
}

} // namespace YTKids
