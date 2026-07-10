// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file editfavoritedialog.h
/// \brief Declares the dialog for editing a saved favourite's endpoint and security.
///

#pragma once

#include "appbasedialog.h"
#include "opcua/connectionprofile.h"

namespace Ui {
class EditFavoriteDialog;
}

///
/// \brief Compact dialog for editing a favourite's server URL and security policy/mode.
///
/// Unlike the full connection dialog, this edits only the two fields that distinguish a
/// favourite, preserving the profile's identity, authentication, and timeout settings.
///
class EditFavoriteDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog from its generated UI and themed styling.
    /// \param parent Parent widget.
    ///
    explicit EditFavoriteDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~EditFavoriteDialog() override;

    ///
    /// \brief Pre-fills the dialog from a favourite, keeping its other settings to write back.
    /// \param profile Favourite being edited.
    ///
    void setProfile(const ConnectionProfile &profile);

    ///
    /// \brief Returns the favourite with the edited URL and security policy/mode applied.
    /// \return Updated profile.
    ///
    ConnectionProfile profile() const;

private:
    void applyStyling();
    void populateSecurityOptions();
    void selectSecurity(const QString &policy, int mode);

    Ui::EditFavoriteDialog *ui;
    ConnectionProfile _profile;
};
