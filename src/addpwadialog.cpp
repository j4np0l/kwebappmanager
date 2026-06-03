#include "addpwadialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPushButton>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

static const QStringList kCategories = {
    QStringLiteral(""),
    QStringLiteral("AudioVideo"),
    QStringLiteral("Development"),
    QStringLiteral("Education"),
    QStringLiteral("Finance"),
    QStringLiteral("Game"),
    QStringLiteral("Graphics"),
    QStringLiteral("Network"),
    QStringLiteral("Office"),
    QStringLiteral("Science"),
    QStringLiteral("Settings"),
    QStringLiteral("Utility"),
};

AddPwaDialog::AddPwaDialog(QWidget *parent)
    : QDialog(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    setWindowTitle(tr("Add Web App"));
    setupUi();
    connect(m_nam, &QNetworkAccessManager::finished, this, &AddPwaDialog::onFaviconFetched);
}

AddPwaDialog::AddPwaDialog(const PwaEntry &entry, QWidget *parent)
    : QDialog(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    setWindowTitle(tr("Edit Web App"));
    setupUi();
    populate(entry);
    connect(m_nam, &QNetworkAccessManager::finished, this, &AddPwaDialog::onFaviconFetched);
}

void AddPwaDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // --- Form ---
    auto *form = new QFormLayout;

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("My App"));
    form->addRow(tr("Name:"), m_nameEdit);

    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText(tr("https://app.example.com"));
    form->addRow(tr("URL:"), m_urlEdit);

    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->addItem(tr("(none)"), QString());
    for (int i = 1; i < kCategories.size(); ++i)
        m_categoryCombo->addItem(kCategories[i], kCategories[i]);
    form->addRow(tr("Category:"), m_categoryCombo);

    m_isolatedCheck = new QCheckBox(tr("Use dedicated browser profile (recommended)"), this);
    m_isolatedCheck->setChecked(true);
    form->addRow(QString(), m_isolatedCheck);

    mainLayout->addLayout(form);

    // --- Icon group ---
    auto *iconGroup = new QGroupBox(tr("Icon"), this);
    auto *iconLayout = new QHBoxLayout(iconGroup);

    m_iconPreview = new QLabel(this);
    m_iconPreview->setFixedSize(64, 64);
    m_iconPreview->setAlignment(Qt::AlignCenter);
    m_iconPreview->setFrameShape(QFrame::StyledPanel);
    m_iconPreview->setText(tr("No icon"));
    iconLayout->addWidget(m_iconPreview);

    auto *iconBtns = new QVBoxLayout;
    m_fetchIconBtn = new QPushButton(tr("Fetch from URL"), this);
    m_chooseIconBtn = new QPushButton(tr("Choose file…"), this);
    iconBtns->addWidget(m_fetchIconBtn);
    iconBtns->addWidget(m_chooseIconBtn);
    iconBtns->addStretch();
    iconLayout->addLayout(iconBtns);

    mainLayout->addWidget(iconGroup);

    // --- Buttons ---
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okBtn = buttonBox->button(QDialogButtonBox::Ok);
    m_okBtn->setEnabled(false);
    mainLayout->addWidget(buttonBox);

    connect(m_fetchIconBtn, &QPushButton::clicked, this, &AddPwaDialog::fetchFavicon);
    connect(m_chooseIconBtn, &QPushButton::clicked, this, &AddPwaDialog::chooseIcon);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &AddPwaDialog::validate);
    connect(m_urlEdit, &QLineEdit::textChanged, this, &AddPwaDialog::validate);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setMinimumWidth(400);
}

void AddPwaDialog::populate(const PwaEntry &e)
{
    m_existingId = e.id;
    m_nameEdit->setText(e.name);
    m_urlEdit->setText(e.url);
    m_isolatedCheck->setChecked(e.isolated);

    const int catIdx = m_categoryCombo->findData(e.category);
    if (catIdx >= 0) m_categoryCombo->setCurrentIndex(catIdx);

    if (!e.iconPath.isEmpty()) {
        m_iconPath = e.iconPath;
        QPixmap px(e.iconPath);
        if (!px.isNull())
            m_iconPreview->setPixmap(px.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    validate();
}

PwaEntry AddPwaDialog::entry() const
{
    PwaEntry e;
    e.id       = m_existingId;
    e.name     = m_nameEdit->text().trimmed();
    e.url      = m_urlEdit->text().trimmed();
    e.category = m_categoryCombo->currentData().toString();
    e.isolated = m_isolatedCheck->isChecked();
    e.iconPath = m_iconPath;
    return e;
}

void AddPwaDialog::fetchFavicon()
{
    QString urlStr = m_urlEdit->text().trimmed();
    if (urlStr.isEmpty()) return;
    QUrl base(urlStr);
    if (!base.isValid()) return;

    const QUrl faviconUrl = QUrl(base.scheme() + QStringLiteral("://") + base.host() + QStringLiteral("/favicon.ico"));
    m_nam->get(QNetworkRequest(faviconUrl));
    m_fetchIconBtn->setEnabled(false);
    m_fetchIconBtn->setText(tr("Fetching…"));
}

void AddPwaDialog::onFaviconFetched(QNetworkReply *reply)
{
    m_fetchIconBtn->setEnabled(true);
    m_fetchIconBtn->setText(tr("Fetch from URL"));
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
        return;

    const QByteArray data = reply->readAll();
    QPixmap px;
    if (!px.loadFromData(data))
        return;

    // Save favicon to app data dir
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                        + QStringLiteral("/icons");
    QDir().mkpath(dir);

    const QString safeName = m_nameEdit->text().trimmed()
                                 .replace(QLatin1Char(' '), QLatin1Char('_'))
                                 .replace(QLatin1Char('/'), QLatin1Char('_'));
    m_iconPath = dir + QStringLiteral("/%1.png").arg(safeName);
    px.save(m_iconPath, "PNG");

    m_iconPreview->setPixmap(px.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void AddPwaDialog::chooseIcon()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Choose Icon"), QString(),
        tr("Images (*.png *.jpg *.jpeg *.svg *.ico *.xpm)"));
    if (path.isEmpty()) return;

    QPixmap px(path);
    if (px.isNull()) return;

    m_iconPath = path;
    m_iconPreview->setPixmap(px.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void AddPwaDialog::validate()
{
    const bool ok = !m_nameEdit->text().trimmed().isEmpty()
                    && QUrl(m_urlEdit->text().trimmed()).isValid()
                    && !m_urlEdit->text().trimmed().isEmpty();
    m_okBtn->setEnabled(ok);
}
