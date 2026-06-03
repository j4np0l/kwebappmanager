#include "mainwindow.h"
#include "addpwadialog.h"
#include "pwamanager.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardAction>
#include <KToolBar>

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPixmap>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
    , m_manager(new PwaManager(this))
{
    setWindowTitle(i18n("Web App Manager"));
    setMinimumSize(640, 480);

    // Central widget
    auto *central = new QWidget(this);
    auto *vbox    = new QVBoxLayout(central);
    vbox->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget(central);

    // Empty state label
    m_emptyLabel = new QLabel(i18n("No web apps yet.\nClick \"Add Web App\" to get started."), central);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet(QStringLiteral("color: palette(mid); font-size: 14px;"));
    m_stack->addWidget(m_emptyLabel);   // index 0

    // List view
    m_list = new QListWidget(central);
    m_list->setIconSize(QSize(48, 48));
    m_list->setSpacing(4);
    m_list->setAlternatingRowColors(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_stack->addWidget(m_list);         // index 1

    vbox->addWidget(m_stack);
    setCentralWidget(central);

    setupActions();

    connect(m_manager, &PwaManager::pwaListChanged, this, &MainWindow::refreshList);
    connect(m_list, &QListWidget::itemSelectionChanged, this, &MainWindow::onSelectionChanged);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &MainWindow::onLaunchPwa);

    refreshList();
    statusBar()->showMessage(i18n("Ready"));
}

void MainWindow::setupActions()
{
    // Standard actions
    KStandardAction::quit(qApp, &QApplication::quit, actionCollection());

    // Add
    auto *addAction = new QAction(QIcon::fromTheme(QStringLiteral("list-add")),
                                  i18n("Add Web App"), this);
    addAction->setToolTip(i18n("Add a new Progressive Web App"));
    actionCollection()->addAction(QStringLiteral("add_pwa"), addAction);
    actionCollection()->setDefaultShortcut(addAction, Qt::CTRL | Qt::Key_N);
    connect(addAction, &QAction::triggered, this, &MainWindow::onAddPwa);

    // Edit
    m_editAction = new QAction(QIcon::fromTheme(QStringLiteral("document-edit")),
                               i18n("Edit"), this);
    m_editAction->setEnabled(false);
    actionCollection()->addAction(QStringLiteral("edit_pwa"), m_editAction);
    connect(m_editAction, &QAction::triggered, this, &MainWindow::onEditPwa);

    // Remove
    m_removeAction = new QAction(QIcon::fromTheme(QStringLiteral("list-remove")),
                                 i18n("Remove"), this);
    m_removeAction->setEnabled(false);
    actionCollection()->addAction(QStringLiteral("remove_pwa"), m_removeAction);
    actionCollection()->setDefaultShortcut(m_removeAction, Qt::Key_Delete);
    connect(m_removeAction, &QAction::triggered, this, &MainWindow::onRemovePwa);

    // Launch
    m_launchAction = new QAction(QIcon::fromTheme(QStringLiteral("media-playback-start")),
                                 i18n("Launch"), this);
    m_launchAction->setEnabled(false);
    actionCollection()->addAction(QStringLiteral("launch_pwa"), m_launchAction);
    actionCollection()->setDefaultShortcut(m_launchAction, Qt::Key_Return);
    connect(m_launchAction, &QAction::triggered, this, &MainWindow::onLaunchPwa);

    setupGUI(Default, QStringLiteral("kwebappmanagerui.rc"));
}

void MainWindow::refreshList()
{
    m_list->clear();
    const auto &pwas = m_manager->pwas();

    for (const auto &e : pwas) {
        auto *item = new QListWidgetItem(m_list);
        item->setText(QStringLiteral("%1\n%2").arg(e.name, e.url));
        item->setData(Qt::UserRole, e.id);

        if (!e.iconPath.isEmpty()) {
            QPixmap px(e.iconPath);
            if (!px.isNull())
                item->setIcon(QIcon(px));
        }
        if (item->icon().isNull())
            item->setIcon(QIcon::fromTheme(QStringLiteral("applications-internet")));
    }

    const bool hasPwas = !pwas.isEmpty();
    m_stack->setCurrentIndex(hasPwas ? 1 : 0);
    statusBar()->showMessage(i18np("1 web app", "%1 web apps", pwas.size()));
    onSelectionChanged();
}

QString MainWindow::selectedId() const
{
    const auto items = m_list->selectedItems();
    if (items.isEmpty()) return {};
    return items.first()->data(Qt::UserRole).toString();
}

void MainWindow::onSelectionChanged()
{
    const bool sel = !selectedId().isEmpty();
    m_editAction->setEnabled(sel);
    m_removeAction->setEnabled(sel);
    m_launchAction->setEnabled(sel);
}

void MainWindow::onAddPwa()
{
    AddPwaDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        if (!m_manager->addPwa(dlg.entry()))
            KMessageBox::error(this, i18n("Failed to create the web app. Check that ~/.local/share/applications/ is writable."));
    }
}

void MainWindow::onEditPwa()
{
    const QString id = selectedId();
    if (id.isEmpty()) return;

    const auto &pwas = m_manager->pwas();
    const auto it = std::find_if(pwas.begin(), pwas.end(),
                                 [&id](const PwaEntry &e) { return e.id == id; });
    if (it == pwas.end()) return;

    AddPwaDialog dlg(*it, this);
    if (dlg.exec() == QDialog::Accepted)
        m_manager->updatePwa(dlg.entry());
}

void MainWindow::onRemovePwa()
{
    const QString id = selectedId();
    if (id.isEmpty()) return;

    const auto &pwas = m_manager->pwas();
    const auto it = std::find_if(pwas.begin(), pwas.end(),
                                 [&id](const PwaEntry &e) { return e.id == id; });
    if (it == pwas.end()) return;

    const auto answer = KMessageBox::questionTwoActions(
        this,
        i18n("Remove \"%1\"? This will also delete its desktop entry.", it->name),
        i18n("Remove Web App"),
        KGuiItem(i18n("Remove"), QStringLiteral("list-remove")),
        KStandardGuiItem::cancel());

    if (answer == KMessageBox::PrimaryAction)
        m_manager->removePwa(id);
}

void MainWindow::onLaunchPwa()
{
    const QString id = selectedId();
    if (!id.isEmpty()) {
        m_manager->launchPwa(id);
        statusBar()->showMessage(i18n("Launching…"), 3000);
    }
}
