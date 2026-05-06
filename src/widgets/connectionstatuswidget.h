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
    explicit ConnectionStatusWidget(QWidget *parent = nullptr);
    ~ConnectionStatusWidget() override;

    void setIcon(const QIcon &icon);
    void setStatusText(const QString &text);

private:
    Ui::ConnectionStatusWidget *ui;
};
