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

class ConnectionController;

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
    /// \brief Subscribes to the controller's connection state and reflects it in the labels.
    /// \param controller Connection controller providing the backend and active profile.
    ///
    void setConnectionController(ConnectionController *controller);

private slots:
    void updateConnectionState(OpcUaConnectionState state);
    void updateClocks();
    void handleServerTime(const QVector<OpcUaDataValue> &values, const QString &error);

private:
    void setupFieldDecorations();
    void setConnectionState(OpcUaConnectionState state,
                            const QString &endpoint = QString(),
                            const QString &securityPolicy = QString(),
                            int securityMode = 0,
                            const QString &sessionName = QString(),
                            const QString &authentication = QString());

    Ui::MainStatusBarWidget *ui;
    ConnectionController *_controller = nullptr;
    bool _serverTimeKnown = false;
    qint64 _serverTimeOffsetMs = 0;
};
