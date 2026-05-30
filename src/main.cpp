#include "mainwindow.h"

#include <KAboutData>
#include <KLocalizedString>
#include <QApplication>

int main(int argc, char *argv[])
{
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
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("applications-internet")));

    // KMainWindow sets WA_DeleteOnClose: the window must be heap-allocated
    // so that Qt's deferred-delete calls free() on a valid heap pointer.
    MainWindow *window = new MainWindow();
    window->show();
    return app.exec();
}
