// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointselectorwidget.h
/// \brief Declares the endpoint selector widget.
///

#pragma once

#include <QWidget>

namespace Ui {
class EndpointSelectorWidget;
}

class QString;

///
/// \brief Widget for selecting the OPC UA endpoint URL.
///
class EndpointSelectorWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the selector with a default localhost endpoint.
    /// \param parent Parent widget.
    ///
    explicit EndpointSelectorWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~EndpointSelectorWidget() override;

    ///
    /// \brief Returns the selected endpoint URL.
    /// \return Current endpoint text.
    ///
    QString currentEndpoint() const;

private:
    Ui::EndpointSelectorWidget *ui;
};
