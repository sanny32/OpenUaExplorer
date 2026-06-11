// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectiondialog.h
/// \brief Declares the OPC UA connection dialog.
///

#pragma once

#include <QList>

#include "dialogs/appbasedialog.h"
#include "opcua/connectionprofile.h"
#include "opcua/opcuatypes.h"

namespace Ui {
class ConnectionDialog;
}

///
/// \brief Dialog for configuring and opening an OPC UA connection.
///
class ConnectionDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    explicit ConnectionDialog(QWidget *parent = nullptr);
    ~ConnectionDialog() override;

    void setClientService(class OpcUaClientService *service);
    ConnectionProfile profile() const;
    QString password() const;
    QString privateKeyPassword() const;

private slots:
    void discoverEndpoints();
    void handleEndpoints(QList<EndpointInfo> endpoints, const QString &error);
    void updateEndpointSelection();
    void updateAuthenticationFields();
    void chooseClientCertificate();
    void generateClientCertificate();
    void validateAndAccept();

private:
    void saveLastEndpointUrl();

    Ui::ConnectionDialog *ui;
    class OpcUaClientService *_service = nullptr;
    QList<EndpointInfo> _endpoints;
    bool _connectAfterDiscovery = false;
    QString _lastEnteredEndpointUrl;
    QString _clientCertificateFile;
    QString _privateKeyFile;
    QString _privateKeyPassword;
};
