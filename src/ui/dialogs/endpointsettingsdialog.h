// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointsettingsdialog.h
/// \brief Declares the read-only endpoint settings inspector dialog.
///

#pragma once

#include <QByteArray>

#include "appbasedialog.h"
#include "opcua/certificateinfo.h"
#include "opcua/connectionprofile.h"

namespace Ui {
class EndpointSettingsDialog;
}

///
/// \brief Read-only inspector of the endpoint used by the active connection.
///
/// Presents the fixed endpoint identity (URL, security policy/mode, authentication,
/// backend) and the session/secure-channel timeouts of the profile that is currently
/// connected. Unlike the connection dialog it never edits or reconnects; it exists so
/// the user can review and copy the settings of the live session.
///
class EndpointSettingsDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog from its generated UI and themed styling.
    /// \param parent Parent widget.
    ///
    explicit EndpointSettingsDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~EndpointSettingsDialog() override;

    ///
    /// \brief Fills the inspector with the values of the active connection profile.
    /// \param profile Profile describing the live connection.
    ///
    void setProfile(const ConnectionProfile &profile);

    ///
    /// \brief Shows the endpoint's server certificate, enabling its details link.
    /// \param der DER-encoded server certificate, or an empty array when none is in use.
    ///
    void setServerCertificate(const QByteArray &der);

private slots:
    void viewServerCertificateDetails();

private:
    void applyStyling();
    void setCertificateStatus(CertificateInfo::Status status);
    void updateServerCertificateFields();

    Ui::EndpointSettingsDialog *ui;
    QByteArray _serverCertificate;
};
