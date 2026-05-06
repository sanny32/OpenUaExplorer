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
    explicit EndpointSelectorWidget(QWidget *parent = nullptr);
    ~EndpointSelectorWidget() override;

    QString currentEndpoint() const;

private:
    Ui::EndpointSelectorWidget *ui;
};
