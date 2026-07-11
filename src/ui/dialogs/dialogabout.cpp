// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dialogabout.cpp
/// \brief Implements the application about dialog.
///

#include <QDesktopServices>
#include <QEvent>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QSvgRenderer>
#include <QSysInfo>
#include <QTabBar>
#include <QTextDocument>
#include <QTextOption>
#include <QUrl>
#include <QVBoxLayout>
#include <QtGlobal>

#include "appicons.h"
#include "dialogabout.h"
#include "ui_dialogabout.h"

///
/// \brief Builds the About dialog and fills its content.
/// \param parent Parent widget.
///
DialogAbout::DialogAbout(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::DialogAbout)
{
    ui->setupUi(this);
    ui->logoLabel->setIcon("app", QSize(128, 128));
    ui->illustrationLabel->setIcon("automation.png", QSize(360, 180));

    setupLayout();
    setupContent();
    setupFonts();

    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

///
/// \brief Destroys the dialog and its generated UI.
///
DialogAbout::~DialogAbout()
{
    delete ui;
}

///
/// \brief Populates the version, links, license, and credits text.
///
void DialogAbout::setupContent()
{
    ui->versionLabel->setText(tr("Version %1").arg(QStringLiteral(APP_VERSION)));

    ui->descriptionLabel->setText(
        tr("OPC UA Client for browsing, monitoring and interacting<br>with industrial data."));

    ui->copyrightLabel->setText(tr("Copyright © %1 Alexandr Ananev").arg(QStringLiteral(BUILD_YEAR)));

    ui->websiteIconLabel->setIcon("website", QSize(16, 16));
    ui->websiteLinkLabel->setText(QStringLiteral("<a href=\"https://sanny32.github.io/OpenUaExplorer\">Website</a>"));
    ui->websiteLinkLabel->setTextFormat(Qt::RichText);
    ui->websiteLinkLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->websiteLinkLabel->setOpenExternalLinks(true);

    ui->githubIconLabel->setIcon("github", QSize(16, 16));
    ui->githubLinkLabel->setText(
        QStringLiteral("<a href=\"https://github.com/sanny32/OpenUaExplorer\">GitHub</a>"));
    ui->githubLinkLabel->setTextFormat(Qt::RichText);
    ui->githubLinkLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->githubLinkLabel->setOpenExternalLinks(true);

    ui->aboutTextLabel->setText(tr(
        "OpenUaExplorer is a modern OPC UA client<br>"
        "designed for engineers and developers.<br><br>"
        "<b>Features:</b><br><br>"
        "&bull;&nbsp;&nbsp;Address Space browsing<br>"
        "&bull;&nbsp;&nbsp;Live data subscriptions<br>"
        "&bull;&nbsp;&nbsp;Data Access monitoring<br>"
        "&bull;&nbsp;&nbsp;Event and History support<br>"
        "&bull;&nbsp;&nbsp;Secure connection management"));

    setupAuthors();

    ui->licenseBrowser->document()->setDefaultStyleSheet(licenseStyleSheet());
    ui->licenseBrowser->document()->setDocumentMargin(2);
    ui->licenseBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->licenseBrowser->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    ui->licenseBrowser->setHtml(licenseHtml());

    setupComponents();
}

///
/// \brief Applies the title font and muted roles to the dialog labels.
///
void DialogAbout::setupFonts()
{
    QFont titleFont = ui->titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    ui->titleLabel->setFont(titleFont);

    ui->versionLabel->setForegroundRole(QPalette::PlaceholderText);
    ui->copyrightLabel->setForegroundRole(QPalette::PlaceholderText);
    ui->linksSeparatorLabel->setForegroundRole(QPalette::PlaceholderText);
    ui->tabWidget->tabBar()->setExpanding(false);
    ui->tabWidget->tabBar()->setMinimumHeight(44);
}

///
/// \brief Keeps the header fixed while the tab area receives vertical resize space.
///
void DialogAbout::setupLayout()
{
    QSizePolicy headerPolicy = ui->headerWidget->sizePolicy();
    headerPolicy.setVerticalPolicy(QSizePolicy::Fixed);
    ui->headerWidget->setSizePolicy(headerPolicy);

    QSizePolicy tabPolicy = ui->tabWidget->sizePolicy();
    tabPolicy.setVerticalPolicy(QSizePolicy::Expanding);
    ui->tabWidget->setSizePolicy(tabPolicy);

    ui->mainLayout->setStretch(0, 0);
    ui->mainLayout->setStretch(1, 1);
    ui->mainLayout->setStretch(2, 0);
}

///
/// \brief Builds the authors list.
///
void DialogAbout::setupAuthors()
{
    const QJsonObject data = aboutData();
    const QStringList sections = { QStringLiteral("authors"), QStringLiteral("contributors") };
    int authorIndex = 0;
    int layoutIndex = 0;

    for (const QString &sectionName : sections) {
        const QJsonArray people = data.value(sectionName).toArray();
        for (const QJsonValue &value : people) {
            const QJsonObject person = value.toObject();
            const QString name = person.value(QStringLiteral("name")).toString();
            if (name.isEmpty()) {
                continue;
            }

            const QString role =
                authorRoleDescription(sectionName, person.value(QStringLiteral("role")).toString());
            const QUrl url(person.value(QStringLiteral("url")).toString());
            ui->authorsLayout->insertWidget(layoutIndex++, createAuthorRow(name, role, url, authorIndex));
            ui->authorsLayout->insertWidget(
                layoutIndex++, createAuthorSeparator(QStringLiteral("authorSeparator%1").arg(authorIndex)));
            ++authorIndex;
        }
    }
}

///
/// \brief Builds the components list.
///
void DialogAbout::setupComponents()
{
    int componentIndex = 0;
    int layoutIndex = 0;

    addComponent(QStringLiteral("Qt"),
                 tr("Using %1 and built against %2").arg(qVersion(), QStringLiteral(QT_VERSION_STR)),
                 tr("Cross-platform application development framework."),
                 QUrl(QStringLiteral("https://www.qt.io")),
                 &layoutIndex,
                 &componentIndex);

    addComponent(QStringLiteral("Qt OPC UA"),
                 QStringLiteral(QT_VERSION_STR),
                 tr("OPC UA client module with the open62541 backend."),
                 QUrl(QStringLiteral("https://doc.qt.io/qt-6/qtopcua-index.html")),
                 &layoutIndex,
                 &componentIndex);

    addComponent(QStringLiteral("QtKeychain"),
                 QStringLiteral(APP_QTKEYCHAIN_VERSION),
                 tr("Secure storage integration for application secrets."),
                 QUrl(QStringLiteral("https://github.com/frankosterfeld/qtkeychain")),
                 &layoutIndex,
                 &componentIndex);

    addComponent(QStringLiteral("OpenSSL"),
                 QStringLiteral(APP_OPENSSL_VERSION),
                 tr("Cryptography and TLS support library."),
                 QUrl(QStringLiteral("https://www.openssl.org")),
                 &layoutIndex,
                 &componentIndex);

    addComponent(QSysInfo::prettyProductName(),
                 QSysInfo::currentCpuArchitecture(),
                 tr("Underlying platform."),
                 {},
                 &layoutIndex,
                 &componentIndex);

#if defined(HAVE_QLEMENTINE_APP_STYLE)
    addComponent(QStringLiteral("Qlementine"),
                 QStringLiteral(QLEMENTINE_VERSION),
                 tr("Modern Qt Widgets style."),
                 QUrl(QStringLiteral("https://github.com/oclero/qlementine")),
                 &layoutIndex,
                 &componentIndex);

    addComponent(QStringLiteral("Qlementine Icons"),
                 QStringLiteral(QLEMENTINE_ICONS_VERSION),
                 tr("Modern Qt Widgets icon theme."),
                 QUrl(QStringLiteral("https://github.com/oclero/qlementine-icons")),
                 &layoutIndex,
                 &componentIndex);
#endif
}

///
/// \brief Loads about dialog data from the application resources.
/// \return Parsed about data object, or an empty object when unavailable.
///
QJsonObject DialogAbout::aboutData() const
{
    QFile file(QStringLiteral(":/res/about.json"));
    if (!file.open(QFile::ReadOnly)) {
        return {};
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return {};
    }

    return document.object();
}

///
/// \brief Maps an about.json role to display text.
/// \param sectionName about.json section name.
/// \param role Role value from about.json.
/// \return Localized role description.
///
QString DialogAbout::authorRoleDescription(const QString &sectionName, const QString &role) const
{
    if (role == QLatin1String("author_maintainer")) {
        return tr("Author");
    }
    if (sectionName == QLatin1String("contributors")) {
        return tr("Maintenance");
    }

    return tr("Contributor");
}

///
/// \brief Adds one component row and separator to the components layout.
/// \param title Component display name.
/// \param version Component version text.
/// \param description Component description.
/// \param url Component homepage URL.
/// \param layoutIndex Current insertion index in the components layout.
/// \param componentIndex Current component row index.
///
void DialogAbout::addComponent(const QString &title,
                               const QString &version,
                               const QString &description,
                               const QUrl &url,
                               int *layoutIndex,
                               int *componentIndex)
{
    ui->componentsLayout->insertWidget(
        (*layoutIndex)++, createComponentRow(title, version, description, url, *componentIndex));
    ui->componentsLayout->insertWidget(
        (*layoutIndex)++,
        createComponentSeparator(QStringLiteral("componentSeparator%1").arg(*componentIndex)));
    ++(*componentIndex);
}

///
/// \brief Creates one authors list row.
/// \param name Contributor display name.
/// \param role Contributor role.
/// \param url Contributor contact URL.
/// \param index Author row index.
/// \return Row widget.
///
QWidget *DialogAbout::createAuthorRow(const QString &name,
                                      const QString &role,
                                      const QUrl &url,
                                      int index)
{
    auto *row = new QWidget(ui->authorsTab);
    row->setObjectName(QStringLiteral("authorRow%1").arg(index));
    row->setFixedHeight(48);

    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(12);

    auto *textLayout = new QVBoxLayout;
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    auto *nameLabel = new QLabel(name, row);
    nameLabel->setObjectName(QStringLiteral("authorNameLabel%1").arg(index));
    QFont nameFont = nameLabel->font();
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);

    auto *roleLabel = new QLabel(role, row);
    roleLabel->setObjectName(QStringLiteral("authorRoleLabel%1").arg(index));

    textLayout->addStretch();
    textLayout->addWidget(nameLabel);
    textLayout->addWidget(roleLabel);
    textLayout->addStretch();

    rowLayout->addLayout(textLayout, 1);
    rowLayout->addWidget(createAuthorContactButton(url, index));

    return row;
}

///
/// \brief Creates the right-side contact marker for an author row.
/// \param url Contact URL.
/// \param index Author row index.
/// \return Contact marker widget.
///
QWidget *DialogAbout::createAuthorContactButton(const QUrl &url, int index)
{
    auto *button = new QPushButton(ui->authorsTab);
    button->setObjectName(QStringLiteral("authorContactButton%1").arg(index));
    button->setFlat(true);
    button->setStyleSheet(iconButtonStyleSheet());
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedSize(24, 24);
    button->setProperty("authorUrl", url);

    if (url.scheme() == QLatin1String("mailto")) {
        button->setText(QStringLiteral("@"));
        button->setToolTip(tr("Email contributor: %1").arg(url.path()));
        QFont contactFont = button->font();
        contactFont.setPointSize(contactFont.pointSize() + 4);
        contactFont.setBold(true);
        button->setFont(contactFont);
    } else if (url.host().contains(QStringLiteral("github"), Qt::CaseInsensitive)) {
        button->setIcon(AppIcons::themed(QStringLiteral("github")));
        button->setIconSize(QSize(18, 18));
        button->setToolTip(tr("Visit GitHub profile: %1").arg(url.toString()));
    } else {
        button->setIcon(AppIcons::themed(QStringLiteral("website")));
        button->setIconSize(QSize(18, 18));
        button->setToolTip(tr("Visit profile: %1").arg(url.toString()));
    }

    button->setVisible(url.isValid() && !url.isEmpty());
    connect(button, &QPushButton::clicked, this, &DialogAbout::openAuthorUrl);
    return button;
}

///
/// \brief Creates a horizontal separator for the authors list.
/// \param objectName Separator object name.
/// \return Separator frame.
///
QFrame *DialogAbout::createAuthorSeparator(const QString &objectName)
{
    auto *separator = new QFrame(ui->authorsTab);
    separator->setObjectName(objectName);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    return separator;
}

///
/// \brief Creates one components list row.
/// \param title Component display name.
/// \param version Component version text.
/// \param description Component description.
/// \param url Component homepage URL.
/// \param index Component row index.
/// \return Row widget.
///
QWidget *DialogAbout::createComponentRow(const QString &title,
                                         const QString &version,
                                         const QString &description,
                                         const QUrl &url,
                                         int index)
{
    auto *row = new QWidget(ui->componentsTab);
    row->setObjectName(QStringLiteral("componentRow%1").arg(index));
    row->setFixedHeight(48);

    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(12);

    auto *textLayout = new QVBoxLayout;
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    auto *titleLayout = new QHBoxLayout;
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(4);

    auto *titleLabel = new QLabel(title, row);
    titleLabel->setObjectName(QStringLiteral("componentTitleLabel%1").arg(index));
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto *versionLabel = new QLabel(version.isEmpty() ? QString() : QStringLiteral("(%1)").arg(version), row);
    versionLabel->setObjectName(QStringLiteral("componentVersionLabel%1").arg(index));
    versionLabel->setForegroundRole(QPalette::PlaceholderText);

    auto *descriptionLabel = new QLabel(description, row);
    descriptionLabel->setObjectName(QStringLiteral("componentDescriptionLabel%1").arg(index));
    descriptionLabel->setWordWrap(true);

    titleLayout->addWidget(titleLabel);
    if (!version.isEmpty()) {
        titleLayout->addWidget(versionLabel);
    }
    titleLayout->addStretch();

    textLayout->addStretch();
    textLayout->addLayout(titleLayout);
    textLayout->addWidget(descriptionLabel);
    textLayout->addStretch();

    rowLayout->addLayout(textLayout, 1);
    rowLayout->addWidget(createComponentContactButton(url, index));

    return row;
}

///
/// \brief Creates the right-side homepage button for a component row.
/// \param url Component homepage URL.
/// \param index Component row index.
/// \return Component contact button.
///
QWidget *DialogAbout::createComponentContactButton(const QUrl &url, int index)
{
    auto *button = new QPushButton(ui->componentsTab);
    button->setObjectName(QStringLiteral("componentContactButton%1").arg(index));
    button->setFlat(true);
    button->setStyleSheet(iconButtonStyleSheet());
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedSize(24, 24);
    button->setIcon(AppIcons::themed(QStringLiteral("website")));
    button->setIconSize(QSize(18, 18));
    button->setProperty("componentUrl", url);
    button->setToolTip(tr("Visit component homepage: %1").arg(url.toString()));
    button->setVisible(url.isValid() && !url.isEmpty());

    connect(button, &QPushButton::clicked, this, &DialogAbout::openComponentUrl);
    return button;
}

///
/// \brief Creates a horizontal separator for the components list.
/// \param objectName Separator object name.
/// \return Separator frame.
///
QFrame *DialogAbout::createComponentSeparator(const QString &objectName)
{
    auto *separator = new QFrame(ui->componentsTab);
    separator->setObjectName(objectName);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    return separator;
}

///
/// \brief Opens the URL stored on the clicked author contact button.
///
void DialogAbout::openAuthorUrl()
{
    const auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) {
        return;
    }

    const QUrl url = button->property("authorUrl").toUrl();
    if (url.isValid() && !url.isEmpty()) {
        QDesktopServices::openUrl(url);
    }
}

///
/// \brief Opens the URL stored on the clicked component homepage button.
///
void DialogAbout::openComponentUrl()
{
    const auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) {
        return;
    }

    const QUrl url = button->property("componentUrl").toUrl();
    if (url.isValid() && !url.isEmpty()) {
        QDesktopServices::openUrl(url);
    }
}

///
/// \brief Builds the stylesheet for flat icon buttons in the about lists.
/// \return CSS keeping the button transparent so the panel shows through.
///
QString DialogAbout::iconButtonStyleSheet() const
{
    return QStringLiteral(
        "QPushButton { background: transparent; border: none; }"
        "QPushButton:hover { background-color: palette(midlight); border-radius: 4px; }"
        "QPushButton:pressed { background-color: palette(mid); border-radius: 4px; }");
}

///
/// \brief Builds the formatted MIT license page.
/// \return Rich text for the license tab.
///
QString DialogAbout::licenseHtml() const
{
    return tr(
        "<h2>MIT License</h2>"
        "<p><b>Copyright %1 Alexandr Ananev</b></p>"
        "<p>Permission is hereby granted, free of charge, to any person obtaining a copy "
        "of this software and associated documentation files (the \"Software\"), to deal "
        "in the Software without restriction, including without limitation the rights to "
        "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of "
        "the Software, and to permit persons to whom the Software is furnished to do so, "
        "subject to the following conditions:</p>"
        "<p>The above copyright notice and this permission notice shall be included in "
        "all copies or substantial portions of the Software.</p>"
        "<p>THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
        "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS "
        "FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR "
        "COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER "
        "IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN "
        "CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.</p>")
        .arg(QStringLiteral(BUILD_YEAR));
}

///
/// \brief Builds palette-aware rich text styling for the license tab.
/// \return CSS for the license text document.
///
QString DialogAbout::licenseStyleSheet() const
{
    const QFont browserFont = ui->licenseBrowser->font();
    const int basePointSize = browserFont.pointSize() > 0 ? browserFont.pointSize() : 10;

    return QStringLiteral(
               "body { font-family: \"%1\"; font-size: %2pt; }"
               "p { margin-top: 0; margin-bottom: 8px; line-height: 135%; }"
               "h2 { margin: 0 0 8px 0; font-size: %3pt; font-weight: 700; }"
               "h3 { margin: 12px 0 8px 0; font-size: %4pt; font-weight: 700; }")
        .arg(browserFont.family(),
             QString::number(basePointSize),
             QString::number(basePointSize + 5),
             QString::number(basePointSize + 1));
}
