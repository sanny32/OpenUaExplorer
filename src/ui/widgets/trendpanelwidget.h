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
    ///
    /// \brief Builds the trend panel and sets its toolbar icons.
    /// \param parent Parent widget.
    ///
    explicit TrendPanelWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the panel and its generated UI.
    ///
    ~TrendPanelWidget() override;

private:
    Ui::TrendPanelWidget *ui;
};
