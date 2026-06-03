#include "pwamanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QStandardPaths>
#include <QUuid>

static PwaEntry entryFromJson(const QJsonObject &obj)
{
    PwaEntry e;
    e.id       = obj[QStringLiteral("id")].toString();
    e.name     = obj[QStringLiteral("name")].toString();
    e.url      = obj[QStringLiteral("url")].toString();
    e.iconPath = obj[QStringLiteral("iconPath")].toString();
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

    // Launch kwebappmanager's own PWA runtime (--app <id>) rather than an external
    // browser, so the app hosts the window and can route external links to the
    // default browser. The WM_CLASS matches the desktop-file name set by the
    // runtime (see main.cpp), so KDE groups the window under this launcher.
    const QString cls = QStringLiteral("kwebapp-") + entry.id;
    const QString execLine = QStringLiteral("\"%1\" --app %2")
                                 .arg(QCoreApplication::applicationFilePath(), entry.id);

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

    // Pre-create the per-PWA profile dir so the runtime's persistent WebEngine
    // profile has somewhere to store cookies/local storage.
    if (entry.isolated)
        QDir().mkpath(profileDir(entry));

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
            if (e.isolated)
                QDir().mkpath(profileDir(e));
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
            // Re-invoke ourselves in PWA runtime mode; the window is hosted by
            // an embedded WebEngine view (see main.cpp / PwaWindow).
            QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                    {QStringLiteral("--app"), id});
            return;
        }
    }
}
