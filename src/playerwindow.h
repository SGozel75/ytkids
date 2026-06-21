#pragma once

#include <QWidget>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QTimer>
#include <QVBoxLayout>

namespace YTKids {

// ── PlayerPage — custom page to intercept navigation ─────────────────────────
class PlayerPage : public QWebEnginePage
{
    Q_OBJECT

public:
    explicit PlayerPage(QWebEngineProfile* profile, QObject* parent = nullptr);

signals:
    void blockedChannelDetected(const QString& channelName);

protected:
    bool acceptNavigationRequest(const QUrl& url,
                                  NavigationType type,
                                  bool isMainFrame) override;
private:
    bool isBlockedUrl(const QUrl& url) const;
};

// ── PlayerWindow ──────────────────────────────────────────────────────────────
class PlayerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerWindow(QWidget* parent = nullptr);
    ~PlayerWindow();

    void startPolling();
    void stopPolling();
    void pauseVideo();
    void loadHome();            // navigate to YT Kids home
    void applyChannelFilters(); // inject block/allow lists into page

signals:
    void videoRemainingUpdated(int seconds);    // fed to TimerController
    void blockedChannelDetected(const QString& name);

public slots:
    void onSessionExpired();        // hard stop — pause + navigate away
    void onVideoShouldFinish();     // let video end, keep polling

private slots:
    void onPollTimer();
    void onPageLoadFinished(bool ok);
    void onVideoRemainingResult(const QVariant& result);

private:
    void setupProfile();
    void injectBridgeScript();
    void injectFilterScript();
    QString buildFilterScript() const;

    QWebEngineView*     m_view      { nullptr };
    PlayerPage*         m_page      { nullptr };
    QWebEngineProfile*  m_profile   { nullptr };
    QTimer              m_pollTimer;

    bool    m_finishing     { false };  // grace mode — waiting for video end
    bool    m_polling       { false };

    // JS bridge — injected on every page load
    static const char* JS_BRIDGE;
};

} // namespace YTKids
