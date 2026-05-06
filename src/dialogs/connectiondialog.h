// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectiondialog.h
/// \brief Declares the OPC UA connection dialog.
///

#pragma once

#include <QDialog>

namespace Ui {
class ConnectionDialog;
}

///
/// \brief Dialog for configuring and opening an OPC UA connection.
///
class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionDialog(QWidget *parent = nullptr);
    ~ConnectionDialog() override;

private:
    Ui::ConnectionDialog *ui;
};
