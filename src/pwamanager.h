#pragma once

#include <QObject>
#include <QList>
#include <QJsonObject>

struct PwaEntry {
    QString id;
    QString name;
    QString url;
    QString iconPath;
    QString browser;   // "chromium", "brave", "google-chrome", "firefox"
    QString category;
    bool isolated;     // use dedicated profile dir
};

class PwaManager : public QObject
{
    Q_OBJECT
public:
    explicit PwaManager(QObject *parent = nullptr);

    const QList<PwaEntry> &pwas() const { return m_pwas; }

    bool addPwa(const PwaEntry &entry);
    bool updatePwa(const PwaEntry &entry);
    bool removePwa(const QString &id);
    void launchPwa(const QString &id);

    static QStringList availableBrowsers();

Q_SIGNALS:
    void pwaListChanged();

private:
    void load();
    void save() const;
    bool writeDesktopFile(const PwaEntry &entry) const;
    void removeDesktopFile(const PwaEntry &entry) const;
    QString dataDir() const;
    QString desktopFilePath(const PwaEntry &entry) const;
    QString profileDir(const PwaEntry &entry) const;

    QList<PwaEntry> m_pwas;
};
