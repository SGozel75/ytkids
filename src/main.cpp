#include "mainwindow.h"

#include <QApplication>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>

int main(int argc, char* argv[])
{
    // ── WebEngine flags — must be set before QApplication ────────────────────
    // XWayland for stable rendering on older Intel iGPUs (Haswell/Iris 5100)
    qputenv("QT_QPA_PLATFORM", "xcb");
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--disable-gpu --no-sandbox --disable-dev-shm-usage");

    QApplication app(argc, argv);

    // ── App metadata (used by QSettings) ─────────────────────────────────────
    app.setApplicationName("ytkids");
    app.setApplicationDisplayName("Kids TV Timer");
    app.setOrganizationName("SGozel");
    app.setOrganizationDomain("codeberg.org/SGozel");
    app.setApplicationVersion("0.1.0");

    // ── Ensure data dirs exist ────────────────────────────────────────────────
    QDir().mkpath(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
    );
    QDir().mkpath(
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
    );

    // ── Main window ───────────────────────────────────────────────────────────
    YTKids::MainWindow win;
    win.show();

    return app.exec();
}
