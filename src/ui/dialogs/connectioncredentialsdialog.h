// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectioncredentialsdialog.h
/// \brief Declares the dialog prompting for a favourite's connection credentials.
///

#pragma once

#include "appbasedialog.h"
#include "opcua/connectionprofile.h"

namespace Ui {
class ConnectionCredentialsDialog;
}

///
/// \brief Prompts for the credentials a favourite needs before connecting.
///
/// Favourites store the authentication method but not the secrets, so connecting one
/// that uses username or certificate authentication opens this dialog to collect the
/// password, or the certificate and private key. It shows only the panel matching the
/// favourite's authentication method.
///
class ConnectionCredentialsDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog from its generated UI and themed styling.
    /// \param parent Parent widget.
    ///
    explicit ConnectionCredentialsDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~ConnectionCredentialsDialog() override;

    ///
    /// \brief Pre-fills the dialog from a favourite and shows the matching credential panel.
    /// \param profile Favourite being connected.
    ///
    void setProfile(const ConnectionProfile &profile);

    ///
    /// \brief Returns the favourite with the entered username and certificate paths applied.
    /// \return Updated profile.
    ///
    ConnectionProfile profile() const;

    ///
    /// \brief Returns the entered username password.
    /// \return Username password.
    ///
    QString password() const;

    ///
    /// \brief Returns the entered private-key password.
    /// \return Private-key password.
    ///
    QString privateKeyPassword() const;

private:
    void applyStyling();
    void setupPasswordToggle();
    void browseCertificate();
    void browsePrivateKey();

    Ui::ConnectionCredentialsDialog *ui;
    ConnectionProfile _profile;
};
