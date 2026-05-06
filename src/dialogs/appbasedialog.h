// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appbasedialog.h
/// \brief Declares a base dialog with application theme integration.
///

#pragma once

#include <QDialog>

class QShowEvent;

///
/// \brief Base dialog that applies shared application dialog behavior.
///
class AppBaseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AppBaseDialog(QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void updateWindowIcon();
};
