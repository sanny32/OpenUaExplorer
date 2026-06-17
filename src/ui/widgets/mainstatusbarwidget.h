// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainstatusbarwidget.h
/// \brief Declares the main status bar widget.
///

#pragma once

#include <QStatusBar>

#include "opcua/opcuatypes.h"

namespace Ui {
class MainStatusBarWidget;
}

///
/// \brief Status bar widget that displays connection state.
///
class MainStatusBarWidget : public QStatusBar
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the status bar and shows the disconnected state.
    /// \param parent Parent widget.
    ///
    explicit MainStatusBarWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the status bar and its generated UI.
    ///
    ~MainStatusBarWidget() override;

    ///
    /// \brief Updates the status text, icon, and security label for the connection state.
    /// \param state Current client state.
    /// \param endpoint Connected endpoint.
    /// \param security Selected security policy.
    ///
    void setConnectionState(OpcUaConnectionState state,
                            const QString &endpoint = QString(),
                            const QString &security = QString());

private:
    Ui::MainStatusBarWidget *ui;
};
