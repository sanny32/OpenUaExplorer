// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendgraphtoolbar.h
/// \brief Declares the toolbar used by a single trend graph.
///

#pragma once

#include <QWidget>

namespace Ui {
class TrendGraphToolbar;
}

///
/// \brief Toolbar for trend graph mode, range and chart commands.
///
class TrendGraphToolbar : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the toolbar and wires its command signals.
    /// \param parent Parent widget.
    ///
    explicit TrendGraphToolbar(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the generated UI.
    ///
    ~TrendGraphToolbar() override;

    ///
    /// \brief Marks Live mode active.
    ///
    void selectLive();

    ///
    /// \brief Marks the matching history range active.
    /// \param windowMs History window length in milliseconds.
    ///
    void selectHistoryWindow(qint64 windowMs);

    ///
    /// \brief Enables the history refresh command.
    /// \param enabled Whether refresh can be clicked.
    ///
    void setRefreshEnabled(bool enabled);

signals:
    ///
    /// \brief Requests a switch to live mode.
    ///
    void liveRequested();

    ///
    /// \brief Requests a switch to a history range.
    /// \param windowMs History window length in milliseconds.
    ///
    void historyRequested(qint64 windowMs);

    ///
    /// \brief Requests that history be re-read from now.
    ///
    void refreshRequested();

    ///
    /// \brief Requests Y-axis auto-scaling.
    ///
    void autoScaleRequested();

    ///
    /// \brief Requests fitting all chart data.
    ///
    void fitRequested();

    ///
    /// \brief Requests chart export.
    ///
    void exportRequested();

    ///
    /// \brief Requests the chart settings dialog.
    ///
    void settingsRequested();

private slots:
    void handleModeClicked(int id);

private:
    Ui::TrendGraphToolbar *ui;
    class QButtonGroup *_modeGroup = nullptr;
};
