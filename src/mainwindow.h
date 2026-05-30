#pragma once

#include <KXmlGuiWindow>
#include <QListWidget>

class PwaManager;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QPushButton;
class QStackedWidget;

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private Q_SLOTS:
    void onAddPwa();
    void onEditPwa();
    void onRemovePwa();
    void onLaunchPwa();
    void onSelectionChanged();
    void refreshList();

private:
    void setupActions();
    QString selectedId() const;

    PwaManager   *m_manager;
    QListWidget  *m_list;
    QLabel       *m_emptyLabel;
    QStackedWidget *m_stack;

    QAction *m_editAction;
    QAction *m_removeAction;
    QAction *m_launchAction;
};
