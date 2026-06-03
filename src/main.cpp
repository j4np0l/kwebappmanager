#include "mainwindow.h"
#include "pwamanager.h"
#include "pwawindow.h"

#include <KAboutData>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QGuiApplication>
#include <QIcon>

#include <algorithm>

int main(int argc, char *argv[])
{
    // Recommended for embedding Qt WebEngine widgets; must precede QApplication.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("kwebappmanager");

    KAboutData about(
        QStringLiteral("kwebappmanager"),
        i18n("Web App Manager"),
        QStringLiteral("0.1.0"),
        i18n("Manage Progressive Web Apps in KDE Plasma"),
        KAboutLicense::GPL_V3,
        i18n("© 2026"));

    about.addAuthor(i18n("Juan"), i18n("Developer"), QStringLiteral("jparias1986@gmail.com"));
    KAboutData::setApplicationData(about);
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("kwebappmanager")));

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("Manage Progressive Web Apps in KDE Plasma"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption appOption(
        QStringLiteral("app"),
        i18n("Run the web app with the given id in PWA window mode."),
        QStringLiteral("id"));
    parser.addOption(appOption);
    parser.process(app);

    // PWA runtime mode: host a single web app in an embedded WebEngine window.
    if (parser.isSet(appOption)) {
        const QString id = parser.value(appOption);
        PwaManager manager;
        const auto &pwas = manager.pwas();
        const auto it = std::find_if(pwas.cbegin(), pwas.cend(),
                                     [&id](const PwaEntry &e) { return e.id == id; });
        if (it == pwas.cend()) {
            qWarning() << "kwebappmanager: no web app with id" << id;
            return 1;
        }

        // Associate the process with this PWA's launcher so KDE shows the right
        // icon/title and groups the window under its own task entry. Go through
        // KAboutData (which also sets QGuiApplication's desktopFileName) so the
        // two stay in sync — setting only QGuiApplication's triggers a warning.
        KAboutData appAbout = KAboutData::applicationData();
        appAbout.setDesktopFileName(QStringLiteral("kwebapp-") + id);
        KAboutData::setApplicationData(appAbout);

        // KMainWindow sets WA_DeleteOnClose; heap-allocate so deferred delete is valid.
        auto *win = new PwaWindow(*it, manager.profileDir(*it));
        win->show();
        return app.exec();
    }

    // KMainWindow sets WA_DeleteOnClose: the window must be heap-allocated
    // so that Qt's deferred-delete calls free() on a valid heap pointer.
    MainWindow *window = new MainWindow();
    window->show();
    return app.exec();
}
