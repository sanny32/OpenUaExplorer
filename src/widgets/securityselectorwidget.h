// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file securityselectorwidget.h
/// \brief Declares the security policy selector widget.
///

#pragma once

#include <QWidget>

namespace Ui {
class SecuritySelectorWidget;
}

class QString;

///
/// \brief Widget for selecting the current OPC UA security policy.
///
class SecuritySelectorWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the selector and populates the security-policy choices.
    /// \param parent Parent widget.
    ///
    explicit SecuritySelectorWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~SecuritySelectorWidget() override;

    ///
    /// \brief Returns the selected security policy.
    /// \return Current security-policy text.
    ///
    QString currentSecurityPolicy() const;

private:
    Ui::SecuritySelectorWidget *ui;
};
