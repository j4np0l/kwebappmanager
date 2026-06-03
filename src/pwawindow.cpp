#include "pwawindow.h"

#include <QAction>
#include <QDesktopServices>
#include <QIcon>
#include <QKeySequence>
#include <QPixmap>
#include <QRegularExpression>
#include <QWebEngineProfile>
#include <QWebEngineView>

// ---------------------------------------------------------------------------
// PwaPage
// ---------------------------------------------------------------------------

PwaPage::PwaPage(QWebEngineProfile *profile, const QUrl &baseUrl, QObject *parent)
    : QWebEnginePage(profile, parent)
    , m_baseHost(baseUrl.host())
{
}

bool PwaPage::sameSite(const QUrl &url) const
{
    const QString host = url.host();
    if (host.isEmpty() || m_baseHost.isEmpty())
        return true; // relative / schemeless target — keep in-app
    if (host == m_baseHost)
        return true;
    // Treat sub-domains of either host as the same site (www.example.com ↔ example.com).
    if (host.endsWith(QLatin1Char('.') + m_baseHost) || m_baseHost.endsWith(QLatin1Char('.') + host))
        return true;
    // Fall back to comparing the last two labels (a rough registrable-domain check).
    const auto lastTwoLabels = [](const QString &h) -> QString {
        const QStringList parts = h.split(QLatin1Char('.'), Qt::SkipEmptyParts);
        if (parts.size() < 2)
            return h;
        return parts.at(parts.size() - 2) + QLatin1Char('.') + parts.last();
    };
    return lastTwoLabels(host) == lastTwoLabels(m_baseHost);
}

bool PwaPage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
    // Only intercept top-level navigations triggered by the user clicking a link.
    // In-page (sub-frame) loads, redirects, form posts, etc. proceed normally.
    if (isMainFrame && type == NavigationTypeLinkClicked) {
        const QString scheme = url.scheme();
        const bool web = (scheme == QLatin1String("http") || scheme == QLatin1String("https"));

        if (web) {
            if (!sameSite(url)) {
                QDesktopServices::openUrl(url); // hand off-site link to default browser
                return false;
            }
        } else if (scheme == QLatin1String("mailto") || scheme == QLatin1String("tel")) {
            QDesktopServices::openUrl(url); // let the OS pick the handler
            return false;
        }
    }
    return true;
}

QWebEnginePage *PwaPage::createWindow(WebWindowType /*type*/)
{
    // target=_blank / window.open(): we never want a second chromeless window.
    // Hand a throwaway page that forwards its first URL to the default browser.
    auto *capture = new QWebEnginePage(profile(), this);
    connect(capture, &QWebEnginePage::urlChanged, this, [capture](const QUrl &url) {
        QDesktopServices::openUrl(url);
        capture->deleteLater();
    });
    return capture;
}

// ---------------------------------------------------------------------------
// PwaWindow
// ---------------------------------------------------------------------------

PwaWindow::PwaWindow(const PwaEntry &entry, const QString &profilePath, QWidget *parent)
    : KMainWindow(parent)
{
    setWindowTitle(entry.name);

    QIcon icon;
    if (!entry.iconPath.isEmpty()) {
        const QPixmap px(entry.iconPath);
        if (!px.isNull())
            icon = QIcon(px);
    }
    if (icon.isNull())
        icon = QIcon::fromTheme(QStringLiteral("applications-internet"));
    setWindowIcon(icon);

    // Isolated apps get a persistent, private profile under their own data dir;
    // non-isolated apps run off-the-record (in-memory, nothing persists). The
    // profile has no parent: ~PwaWindow deletes it explicitly, after the page.
    if (entry.isolated) {
        m_profile = new QWebEngineProfile(QStringLiteral("kwebapp-") + entry.id);
        m_profile->setPersistentStoragePath(profilePath);
        m_profile->setCachePath(profilePath + QStringLiteral("/cache"));
        m_profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    } else {
        m_profile = new QWebEngineProfile;
    }

    // Some sites (OneDrive, Google, …) sniff the user agent and reject anything
    // carrying the "QtWebEngine/<ver>" token ("requires Google Chrome 85+").
    // Strip that token so we present as the plain Chrome the engine is built on.
    QString ua = m_profile->httpUserAgent();
    ua.remove(QRegularExpression(QStringLiteral("QtWebEngine/\\S+\\s*")));
    m_profile->setHttpUserAgent(ua.simplified());

    m_view = new QWebEngineView(this);
    // Page is parented to the view (which owns it); popup-capture pages are
    // parented to this page. Deleting the view thus tears down every page before
    // the profile is released (see ~PwaWindow).
    const QUrl url(entry.url);
    auto *page = new PwaPage(m_profile, url, m_view);
    m_view->setPage(page);
    setCentralWidget(m_view);

    // Same-site navigation stays in-app, so offer Back/Forward (no visible chrome).
    m_view->addAction(m_view->pageAction(QWebEnginePage::Back));
    m_view->addAction(m_view->pageAction(QWebEnginePage::Forward));
    m_view->pageAction(QWebEnginePage::Back)->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));
    m_view->pageAction(QWebEnginePage::Forward)->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Right));

    m_view->setUrl(url);

    // Default size, then let KMainWindow restore (and keep saving) per-PWA geometry.
    resize(1024, 768);
    setAutoSaveSettings(QStringLiteral("PwaWindow-") + entry.id, true);
}

PwaWindow::~PwaWindow()
{
    // Order matters: a QWebEnginePage must be destroyed before its profile.
    // Delete the view (which owns the page and any popup-capture pages) first,
    // then the profile — otherwise WebEngine warns and may misbehave.
    delete m_view;
    m_view = nullptr;
    delete m_profile;
    m_profile = nullptr;
}
