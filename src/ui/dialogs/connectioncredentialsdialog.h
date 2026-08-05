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
/// \brief Prompts for the credentials a connection needs and none are stored for.
///
/// Favourites and saved sessions keep the authentication method but not the secrets, so
/// connecting one that uses username or certificate authentication opens this dialog to
/// collect the password, or the certificate and private key. It shows only the panel
/// matching the profile's authentication method, and offers to remember what was typed
/// so the next connection needs no prompt.
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
    /// \brief Explains that the previous credentials were turned down by the server.
    /// \param rejection Reason reported by the server; an empty string restores the hint.
    ///
    void setRejection(const QString &rejection);

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

    ///
    /// \brief Reports whether the entered secrets should be kept in the credential store.
    /// \return True when the user asked to be remembered.
    ///
    bool rememberCredentials() const;

private:
    void applyStyling();
    void setupPasswordToggle();
    void showAuthPage(QWidget *page);
    void browseCertificate();
    void browsePrivateKey();

    Ui::ConnectionCredentialsDialog *ui;
    ConnectionProfile _profile;
};
