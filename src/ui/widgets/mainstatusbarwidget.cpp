// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainstatusbarwidget.cpp
/// \brief Implements the main status bar widget.
///

#include <QWidget>

#include "mainstatusbarwidget.h"
#include "opcua/connectioncontroller.h"
#include "opcua/opcuaclientservice.h"
#include "ui_mainstatusbarwidget.h"

///
/// \brief Builds the status bar and shows the disconnected state.
/// \param parent Parent widget.
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

namespace {

///
/// \brief Builds the abbreviated "policy / mode" security text shown in the status bar.
/// \param securityPolicy Security policy URI (or short name).
/// \param securityMode Message security mode value (None=1, Sign=2, Sign & Encrypt=3).
/// \return Compact security description, or an empty string when no policy is set.
///
QString securitySummary(const QString &securityPolicy, int securityMode)
{
    if (securityPolicy.isEmpty())
        return QString();

    const QString policy = securityPolicy.section(QLatin1Char('#'), -1);
    QString mode;
    switch (securityMode) {
    case 2: mode = MainStatusBarWidget::tr("Sign"); break;
    case 3: mode = MainStatusBarWidget::tr("Sign & Encrypt"); break;
    default: break; // None / Invalid: the policy name already reads "None".
    }
    return mode.isEmpty() ? policy : QStringLiteral("%1 / %2").arg(policy, mode);
}

}

///
/// \brief Subscribes to the controller's connection state and reflects it in the labels.
/// \param controller Connection controller providing the client service and active profile.
///
void MainStatusBarWidget::setConnectionController(ConnectionController *controller)
{
    _controller = controller;
    connect(controller->clientService(), &OpcUaClientService::stateChanged,
            this, &MainStatusBarWidget::updateConnectionState);
    updateConnectionState(controller->clientService()->state());
}

///
/// \brief Refreshes the labels from the current state and the active profile.
/// \param state Current client state.
///
void MainStatusBarWidget::updateConnectionState(OpcUaConnectionState state)
{
    const ConnectionProfile &profile = _controller->activeProfile();
    const bool connected = state == OpcUaConnectionState::Connected;
    const bool active = connected || state == OpcUaConnectionState::Connecting;
    setConnectionState(state, profile.endpointUrl,
                       active ? profile.securityPolicy : QString(),
                       active ? profile.securityMode : 0,
                       connected ? profile.sessionName : QString());
}

///
/// \brief Updates the status text, icon, security, and session labels for the connection state.
/// \param state Current client state.
/// \param endpoint Connected endpoint.
/// \param securityPolicy Selected security policy URI.
/// \param securityMode Selected message security mode value.
/// \param sessionName Active session name.
///
void MainStatusBarWidget::setConnectionState(OpcUaConnectionState state,
                                             const QString &endpoint,
                                             const QString &securityPolicy,
                                             int securityMode,
                                             const QString &sessionName)
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
    const QString security = securitySummary(securityPolicy, securityMode);
    ui->statusIconLabel->setIcon(icon, QSize(12, 12));
    ui->connectionLabel->setText(text);
    ui->securityLabel->setText(security.isEmpty()
        ? tr("Security: -")
        : tr("Security: %1").arg(security));
    ui->sessionLabel->setText(sessionName.isEmpty()
        ? tr("Session: -")
        : tr("Session: %1").arg(sessionName));
    ui->serverTimeLabel->setText(tr("Server Time: -"));
    ui->publishingIntervalLabel->setText(tr("Publishing Interval: -"));
}

///
/// \brief Destroys the status bar and its generated UI.
///
MainStatusBarWidget::~MainStatusBarWidget()
{
    delete ui;
}
