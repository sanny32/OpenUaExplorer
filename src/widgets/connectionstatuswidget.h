// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectionstatuswidget.h
/// \brief Declares the connection status widget.
///

#pragma once

#include <QWidget>

namespace Ui {
class ConnectionStatusWidget;
}

class QIcon;

///
/// \brief Widget that shows connection status text and icon.
///
class ConnectionStatusWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the connection status widget.
    /// \param parent Parent widget.
    ///
    explicit ConnectionStatusWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~ConnectionStatusWidget() override;

    ///
    /// \brief Sets the status icon.
    /// \param icon Icon shown next to the status text.
    ///
    void setIcon(const QIcon &icon);

    ///
    /// \brief Sets the status text.
    /// \param text Text to display.
    ///
    void setStatusText(const QString &text);

private:
    Ui::ConnectionStatusWidget *ui;
};
