#include "pwamanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUuid>

// Writes user.js + userChrome.css into a Firefox profile to hide all browser chrome.
// Must be called after the profile directory has been created.
static void setupFirefoxProfile(const QString &profilePath)
{
    // Opt the profile into userChrome.css support (disabled by default since Firefox 69).
    // Also force server-side decorations so KWin provides the titlebar with min/max/close
    // buttons independently of what we hide inside the browser chrome.
    QFile userJs(profilePath + QStringLiteral("/user.js"));
    if (userJs.open(QIODevice::WriteOnly | QIODevice::Text)) {
        userJs.write(
            "user_pref(\"toolkit.legacyUserProfileCustomizations.stylesheets\", true);\n"
            // 0 = delegate titlebar/window-buttons to the window manager (KWin), not Firefox
            "user_pref(\"browser.tabs.inTitlebar\", 0);\n"
            "user_pref(\"browser.link.open_newwindow\", 1);\n"
        );
    }

    QDir().mkpath(profilePath + QStringLiteral("/chrome"));
    QFile css(profilePath + QStringLiteral("/chrome/userChrome.css"));
    if (css.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // Hide each toolbar individually rather than the whole #navigator-toolbox container.
        // Hiding the container also removes Firefox's CSD window-control buttons; hiding
        // the child bars only removes the browser UI while leaving the toolbox wrapper intact
        // (needed so the WM-provided titlebar sits flush against the page content).
        css.write(
            "/* kwebappmanager: PWA mode */\n"
            "#toolbar-menubar  { display: none !important; }\n"
            "#TabsToolbar      { display: none !important; }\n"
            "#nav-bar          { display: none !important; }\n"
            "#PersonalToolbar  { display: none !important; }\n"
        );
    }
}

// Produces a WM_CLASS-safe string: ASCII, no spaces, used for --class= and StartupWMClass=
static QString wmClass(const PwaEntry &entry)
{
    QString cls = entry.name;
    cls.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9]")), QStringLiteral("-"));
    cls.replace(QRegularExpression(QStringLiteral("-{2,}")), QStringLiteral("-"));
    cls.remove(QRegularExpression(QStringLiteral("^-|-$")));
    return cls.isEmpty() ? QStringLiteral("kwebapp-") + entry.id : cls;
}

static PwaEntry entryFromJson(const QJsonObject &obj)
{
    PwaEntry e;
    e.id       = obj[QStringLiteral("id")].toString();
    e.name     = obj[QStringLiteral("name")].toString();
    e.url      = obj[QStringLiteral("url")].toString();
    e.iconPath = obj[QStringLiteral("iconPath")].toString();
    e.browser  = obj[QStringLiteral("browser")].toString();
    e.category = obj[QStringLiteral("category")].toString();
    e.isolated = obj[QStringLiteral("isolated")].toBool(true);
    return e;
}

static QJsonObject entryToJson(const PwaEntry &e)
{
    QJsonObject obj;
    obj[QStringLiteral("id")]       = e.id;
    obj[QStringLiteral("name")]     = e.name;
    obj[QStringLiteral("url")]      = e.url;
    obj[QStringLiteral("iconPath")] = e.iconPath;
    obj[QStringLiteral("browser")]  = e.browser;
    obj[QStringLiteral("category")] = e.category;
    obj[QStringLiteral("isolated")] = e.isolated;
    return obj;
}

PwaManager::PwaManager(QObject *parent)
    : QObject(parent)
{
    load();
}

QString PwaManager::dataDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString PwaManager::desktopFilePath(const PwaEntry &entry) const
{
    const QString appsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    return appsDir + QStringLiteral("/kwebapp-%1.desktop").arg(entry.id);
}

QString PwaManager::profileDir(const PwaEntry &entry) const
{
    return dataDir() + QStringLiteral("/profiles/") + entry.id;
}

void PwaManager::load()
{
    const QString path = dataDir() + QStringLiteral("/pwas.json");
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    const QJsonArray arr = doc.array();
    m_pwas.clear();
    for (const auto &val : arr)
        m_pwas.append(entryFromJson(val.toObject()));
}

void PwaManager::save() const
{
    const QString dir = dataDir();
    QDir().mkpath(dir);

    QJsonArray arr;
    for (const auto &e : m_pwas)
        arr.append(entryToJson(e));

    QFile f(dir + QStringLiteral("/pwas.json"));
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(arr).toJson());
}

bool PwaManager::writeDesktopFile(const PwaEntry &entry) const
{
    const QString appsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir().mkpath(appsDir);

    QString execLine;
    const QString browser = entry.browser.isEmpty() ? QStringLiteral("chromium") : entry.browser;
    const QString cls = wmClass(entry);

    if (browser == QLatin1String("firefox")) {
        // --name sets the WM instance; Firefox maps it to the class too on most builds
        execLine = QStringLiteral("firefox --new-instance --name %1 --profile %2 %3")
                       .arg(cls, profileDir(entry), entry.url);
    } else {
        // --class forces the WM_CLASS so KDE can match this window to the .desktop file
        const QString profileArg = entry.isolated
            ? QStringLiteral(" --user-data-dir=%1").arg(profileDir(entry))
            : QString();
        execLine = QStringLiteral("%1 --app=%2 --class=%3%4")
                       .arg(browser, entry.url, cls, profileArg);
    }

    QString icon = entry.iconPath.isEmpty()
        ? QStringLiteral("applications-internet")
        : entry.iconPath;

    const QString category = entry.category.isEmpty()
        ? QStringLiteral("Network;WebBrowser;")
        : entry.category + QStringLiteral(";Network;WebBrowser;");

    const QString content = QStringLiteral(
        "[Desktop Entry]\n"
        "Version=1.0\n"
        "Type=Application\n"
        "Name=%1\n"
        "Comment=Progressive Web App\n"
        "Exec=%2\n"
        "Icon=%3\n"
        "Categories=%4\n"
        "StartupWMClass=%6\n"
        "StartupNotify=true\n"
        "X-KWebAppManager-ID=%5\n"
    ).arg(entry.name, execLine, icon, category, entry.id, cls);

    QFile f(desktopFilePath(entry));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    f.write(content.toUtf8());
    return true;
}

void PwaManager::removeDesktopFile(const PwaEntry &entry) const
{
    QFile::remove(desktopFilePath(entry));
}

bool PwaManager::addPwa(const PwaEntry &entryIn)
{
    PwaEntry entry = entryIn;
    if (entry.id.isEmpty())
        entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);

    if (!writeDesktopFile(entry))
        return false;

    if (entry.isolated) {
        const QString prof = profileDir(entry);
        QDir().mkpath(prof);
        const QString browser = entry.browser.isEmpty() ? QStringLiteral("chromium") : entry.browser;
        if (browser == QLatin1String("firefox"))
            setupFirefoxProfile(prof);
    }

    m_pwas.append(entry);
    save();
    Q_EMIT pwaListChanged();
    return true;
}

bool PwaManager::updatePwa(const PwaEntry &entry)
{
    for (auto &e : m_pwas) {
        if (e.id == entry.id) {
            removeDesktopFile(e);
            e = entry;
            writeDesktopFile(e);
            if (e.isolated) {
                const QString prof = profileDir(e);
                QDir().mkpath(prof);
                const QString browser = e.browser.isEmpty() ? QStringLiteral("chromium") : e.browser;
                if (browser == QLatin1String("firefox"))
                    setupFirefoxProfile(prof);
            }
            save();
            Q_EMIT pwaListChanged();
            return true;
        }
    }
    return false;
}

bool PwaManager::removePwa(const QString &id)
{
    for (int i = 0; i < m_pwas.size(); ++i) {
        if (m_pwas[i].id == id) {
            removeDesktopFile(m_pwas[i]);
            // Remove isolated profile if present
            const QString prof = profileDir(m_pwas[i]);
            if (m_pwas[i].isolated && QDir(prof).exists())
                QDir(prof).removeRecursively();
            m_pwas.removeAt(i);
            save();
            Q_EMIT pwaListChanged();
            return true;
        }
    }
    return false;
}

void PwaManager::launchPwa(const QString &id)
{
    for (const auto &e : m_pwas) {
        if (e.id == id) {
            const QString browser = e.browser.isEmpty() ? QStringLiteral("chromium") : e.browser;
            QStringList args;

            const QString cls = wmClass(e);
            if (browser == QLatin1String("firefox")) {
                args << QStringLiteral("--new-instance")
                     << QStringLiteral("--name") << cls
                     << QStringLiteral("--profile") << profileDir(e)
                     << e.url;
            } else {
                args << QStringLiteral("--app=") + e.url
                     << QStringLiteral("--class=") + cls;
                if (e.isolated)
                    args << QStringLiteral("--user-data-dir=") + profileDir(e);
            }

            QProcess::startDetached(browser, args);
            return;
        }
    }
}

QStringList PwaManager::availableBrowsers()
{
    static const QStringList candidates = {
        QStringLiteral("chromium"),
        QStringLiteral("chromium-browser"),
        QStringLiteral("google-chrome"),
        QStringLiteral("google-chrome-stable"),
        QStringLiteral("brave"),
        QStringLiteral("brave-browser"),
        QStringLiteral("microsoft-edge"),
        QStringLiteral("firefox"),
    };

    QStringList found;
    for (const QString &b : candidates) {
        if (!QStandardPaths::findExecutable(b).isEmpty())
            found << b;
    }
    return found;
}
