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
    /// \brief Marks the custom interval command active.
    ///
    void selectCustom();

    ///
    /// \brief Shows the custom interval and its length beside the Custom button.
    /// \param range Interval range text; an empty string hides the display.
    /// \param duration Human-readable interval length shown in the badge.
    ///
    void setInterval(const QString &range, const QString &duration);

    ///
    /// \brief Enables the history refresh command.
    /// \param enabled Whether refresh can be clicked.
    ///
    void setRefreshEnabled(bool enabled);

    ///
    /// \brief Syncs the live play/pause control without emitting a request.
    /// \param paused True to show the paused (resume) state.
    ///
    void setLivePaused(bool paused);

signals:
    ///
    /// \brief Requests a switch to live mode.
    ///
    void liveRequested();

    ///
    /// \brief Requests that live streaming be paused or resumed.
    /// \param paused True to pause, false to resume.
    ///
    void livePauseToggled(bool paused);

    ///
    /// \brief Requests a switch to a history range.
    /// \param windowMs History window length in milliseconds.
    ///
    void historyRequested(qint64 windowMs);

    ///
    /// \brief Requests that the custom interval dialog be opened.
    ///
    void customIntervalRequested();

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

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void handleModeClicked(int id);
    void handleLivePauseClicked();

private:
    void updateLivePauseButton();
    void applyIntervalStyling();

    Ui::TrendGraphToolbar *ui;
    class QButtonGroup *_modeGroup = nullptr;
    bool _livePaused = false;
};
