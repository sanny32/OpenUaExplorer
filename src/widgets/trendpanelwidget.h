// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendpanelwidget.h
/// \brief Declares the trend panel widget.
///

#pragma once

#include <QWidget>

namespace Ui {
class TrendPanelWidget;
}

///
/// \brief Hosts trend graph controls and actions.
///
class TrendPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrendPanelWidget(QWidget *parent = nullptr);
    ~TrendPanelWidget() override;

private:
    Ui::TrendPanelWidget *ui;
};
