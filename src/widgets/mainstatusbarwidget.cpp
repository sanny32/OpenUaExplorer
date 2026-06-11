// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainstatusbarwidget.cpp
/// \brief Implements the main status bar widget.
///

#include <QWidget>

#include "mainstatusbarwidget.h"
#include "ui_mainstatusbarwidget.h"

///
/// \brief MainStatusBarWidget::MainStatusBarWidget
/// \param parent
///
MainStatusBarWidget::MainStatusBarWidget(QWidget *parent)
    : QStatusBar(parent)
    , ui(new Ui::MainStatusBarWidget)
{
    auto *content = new QWidget(this);
    ui->setupUi(content);
    setConnectionState(OpcUaConnectionState::Disconnected);
    addWidget(content, 1);
}

///
/// \brief MainStatusBarWidget::setConnectionState
/// \param state Current client state.
/// \param endpoint Connected endpoint.
/// \param security Selected security policy.
///
void MainStatusBarWidget::setConnectionState(OpcUaConnectionState state,
                                             const QString &endpoint,
                                             const QString &security)
{
    QString text;
    QString icon = QStringLiteral("disconnected.svg");
    switch (state) {
    case OpcUaConnectionState::Unavailable:
        text = tr("OPC UA unavailable");
        break;
    case OpcUaConnectionState::Discovering:
        text = tr("Discovering endpoints");
        break;
    case OpcUaConnectionState::Connecting:
        text = tr("Connecting: %1").arg(endpoint);
        icon = QStringLiteral("connect.svg");
        break;
    case OpcUaConnectionState::Connected:
        text = tr("Connected: %1").arg(endpoint);
        icon = QStringLiteral("connected.svg");
        break;
    case OpcUaConnectionState::Closing:
        text = tr("Disconnecting");
        icon = QStringLiteral("disconnect.svg");
        break;
    case OpcUaConnectionState::Disconnected:
        text = tr("Disconnected");
        break;
    }
    ui->statusIconLabel->setIcon(icon, QSize(12, 12));
    ui->connectionLabel->setText(text);
    ui->securityLabel->setText(security.isEmpty()
        ? tr("Security: -")
        : tr("Security: %1").arg(security));
    ui->sessionLabel->setText(tr("Session: -"));
    ui->serverTimeLabel->setText(tr("Server Time: -"));
    ui->publishingIntervalLabel->setText(tr("Publishing Interval: -"));
}

///
/// \brief MainStatusBarWidget::~MainStatusBarWidget
///
MainStatusBarWidget::~MainStatusBarWidget()
{
    delete ui;
}
