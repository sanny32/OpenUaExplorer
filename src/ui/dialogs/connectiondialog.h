// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectiondialog.h
/// \brief Declares the OPC UA connection dialog.
///

#pragma once

#include <QList>

#include "dialogs/appbasedialog.h"
#include "opcua/endpointhistorystore.h"
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
    ///
    /// \brief Builds the dialog and initialises its history, certificate panels, and controls.
    /// \param parent Parent widget.
    ///
    explicit ConnectionDialog(QWidget *parent = nullptr);

    ///
    /// \brief Saves the last endpoint URL and destroys the dialog.
    ///
    ~ConnectionDialog() override;

    ///
    /// \brief Sets the client service used for discovery and subscribes to its results.
    /// \param service OPC UA client service.
    ///
    void setClientService(class OpcUaClientService *service);

    ///
    /// \brief Pre-fills the dialog from a saved profile so a favourite opens ready to connect.
    /// \param profile Profile whose endpoint, authentication, and timeouts are applied.
    ///
    void setProfile(const ConnectionProfile &profile);

    ///
    /// \brief Builds a connection profile from the dialog's current selections.
    /// \return Connection settings selected by the user.
    ///
    ConnectionProfile profile() const;

    ///
    /// \brief Returns the entered username password.
    /// \return Username password.
    ///
    QString password() const;

    ///
    /// \brief Returns the password for the imported private key.
    /// \return Imported private key password.
    ///
    QString privateKeyPassword() const;

private slots:
    void discoverEndpoints();
    void handleEndpoints(QList<EndpointInfo> endpoints, const QString &error);
    void updateEndpointSelection();
    void updateAuthenticationFields();
    void browseCertificateAuthCertificate();
    void browseCertificateAuthPrivateKey();
    void chooseClientCertificate();
    void generateClientCertificate();
    void handleClientCertificateAction();
    void viewClientCertificateDetails();
    void viewServerCertificateDetails();
    void validateAndAccept();

private:
    void setupEndpointHistory();
    void setupCertificatePanels();
    void setupControls();
    void setupConnections();
    void applySessionDefaults();
    void saveSessionDefaults();
    int currentAuthentication() const;
    void saveLastEndpointUrl();
    void resetDiscovery();
    void selectGeneratedClientCertificate();
    void updateClientCertificate();
    void updateClientCertificateAction();
    void showCertificateDetails(const QByteArray &certificate,
                                const QString &certificatePath = QString());

    Ui::ConnectionDialog *ui;
    class OpcUaClientService *_service = nullptr;
    EndpointHistoryStore _endpointHistoryStore;
    QString _presetId;
    bool _connectAfterDiscovery = false;
    QString _lastEnteredEndpointUrl;
    QString _clientCertificateFile;
    QString _privateKeyFile;
    int _selectedSecurityModeValue = 1;
};
