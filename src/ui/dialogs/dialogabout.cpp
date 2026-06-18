// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dialogabout.cpp
/// \brief Implements the application about dialog.
///

#include <QEvent>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QStyle>
#include <QSvgRenderer>
#include <QTabBar>

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

    ui->copyrightLabel->setText(tr("Copyright 2026 Alexandr Ananev"));

    ui->websiteIconLabel->setIcon("website", QSize(16, 16));
    ui->websiteLinkLabel->setText(QStringLiteral("<a href=\"https://ananev.org\">Website</a>"));
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

    ui->authorsLabel->setText(tr(
        "<b>OpenUaExplorer contributors</b><br>"
        "Alexandr Ananev &lt;mail@ananev.org&gt;<br>"
        "Thanks to everyone who reports issues, tests builds and improves the project."));

    ui->licenseBrowser->setHtml(tr(
        "<b>MIT License</b><br>"
        "Copyright 2026 Alexandr Ananev<br>"
        "Permission is hereby granted, free of charge, to any person obtaining a copy "
        "of this software and associated documentation files to deal in the software "
        "without restriction, including without limitation the rights to use, copy, "
        "modify, merge, publish, distribute, sublicense, and/or sell copies of the "
        "software, subject to the conditions in the project LICENSE file.<br>"
        "The software is provided \"as is\", without warranty of any kind."));

    ui->creditsLabel->setText(tr(
        "<b>Built with</b><br>"
        "Qt Framework<br>"
        "OPC UA client technologies<br>"
        "<b>Visual identity</b><br>"
        "OpenUaExplorer application artwork and theme-aware icons."));
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
