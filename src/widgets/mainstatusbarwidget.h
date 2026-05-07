// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainstatusbarwidget.h
/// \brief Declares the main status bar widget.
///

#pragma once

#include <QStatusBar>

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
    explicit MainStatusBarWidget(QWidget *parent = nullptr);
    ~MainStatusBarWidget() override;

private:
    Ui::MainStatusBarWidget *ui;
    QWidget *_contentWidget;
};
