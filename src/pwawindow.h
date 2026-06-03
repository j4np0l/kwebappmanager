#pragma once

#include "pwamanager.h"

#include <KMainWindow>
#include <QUrl>
#include <QWebEnginePage>

class QWebEngineView;
class QWebEngineProfile;

// QWebEnginePage subclass that keeps same-site navigation inside the PWA window
// and routes off-site link clicks (and target=_blank / window.open popups) to the
// system default browser via QDesktopServices::openUrl().
class PwaPage : public QWebEnginePage
{
    Q_OBJECT
public:
    PwaPage(QWebEngineProfile *profile, const QUrl &baseUrl, QObject *parent = nullptr);

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;
    QWebEnginePage *createWindow(WebWindowType type) override;

private:
    // True if url belongs to the PWA's own site (same/related host) and should
    // navigate in-app; false if it should open in the default browser.
    bool sameSite(const QUrl &url) const;

    QString m_baseHost;
};

// Top-level window hosting a single PWA in an embedded web view. One instance
// per process — used when kwebappmanager is launched with --app <id>.
class PwaWindow : public KMainWindow
{
    Q_OBJECT
public:
    // profilePath is the per-PWA storage dir (PwaManager::profileDir), used as the
    // persistent WebEngine profile location when the app is isolated.
    PwaWindow(const PwaEntry &entry, const QString &profilePath, QWidget *parent = nullptr);
    ~PwaWindow() override;

private:
    QWebEngineView *m_view = nullptr;
    QWebEngineProfile *m_profile = nullptr;
};
