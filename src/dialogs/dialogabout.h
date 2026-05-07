// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dialogabout.h
/// \brief Declares the application about dialog.
///

#pragma once

#include <QSize>

#include "dialogs/appbasedialog.h"

namespace Ui {
class DialogAbout;
}

///
/// \brief Dialog that presents application identity, links and project information.
///
class DialogAbout : public AppBaseDialog
{
    Q_OBJECT

public:
    explicit DialogAbout(QWidget *parent = nullptr);
    ~DialogAbout() override;

private:
    void setupContent();
    void setupFonts();

private:
    Ui::DialogAbout *ui;
};
