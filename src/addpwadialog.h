#pragma once

#include "pwamanager.h"
#include <QDialog>

class QLineEdit;
class QComboBox;
class QCheckBox;
class QLabel;
class QPushButton;
class QNetworkAccessManager;
class QNetworkReply;

class AddPwaDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddPwaDialog(QWidget *parent = nullptr);
    explicit AddPwaDialog(const PwaEntry &entry, QWidget *parent = nullptr);

    PwaEntry entry() const;

private Q_SLOTS:
    void fetchFavicon();
    void onFaviconFetched(QNetworkReply *reply);
    void chooseIcon();
    void validate();

private:
    void setupUi();
    void populate(const PwaEntry &e);

    QLineEdit *m_nameEdit;
    QLineEdit *m_urlEdit;
    QComboBox *m_categoryCombo;
    QCheckBox *m_isolatedCheck;
    QLabel    *m_iconPreview;
    QPushButton *m_fetchIconBtn;
    QPushButton *m_chooseIconBtn;
    QPushButton *m_okBtn;

    QString m_iconPath;
    QString m_existingId;
    QNetworkAccessManager *m_nam;
};
