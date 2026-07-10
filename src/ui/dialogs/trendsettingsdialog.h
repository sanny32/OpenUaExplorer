// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendsettingsdialog.h
/// \brief Declares the per-chart trend settings dialog.
///

#pragma once

#include <QColor>
#include <QVector>

#include "appbasedialog.h"
#include "models/subscriptionitem.h"
#include "trendsettings.h"

namespace Ui {
class TrendSettingsDialog;
}

class QButtonGroup;
class QStandardItemModel;

///
/// \brief Edits one trend chart's display options, range and per-series colours.
///
/// The dialog mirrors the chart's current state, lets the user adjust display
/// toggles, the active period, live auto-scroll, and each series' visibility and
/// colour, and exposes the edited values for the host to apply on accept. Reset
/// restores the documented defaults.
///
class TrendSettingsDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog from its generated UI and themed styling.
    /// \param parent Parent widget.
    ///
    explicit TrendSettingsDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~TrendSettingsDialog() override;

    ///
    /// \brief Populates the live-subscription combo with the known subscriptions.
    /// \param subscriptions Current subscriptions in row order.
    ///
    void setSubscriptions(const QVector<SubscriptionItem> &subscriptions);

    ///
    /// \brief Pre-fills the display, range and auto-scroll controls.
    /// \param settings Current chart settings.
    ///
    void setDisplaySettings(const TrendDisplaySettings &settings);

    ///
    /// \brief Returns the edited display, range and auto-scroll settings.
    /// \return Updated settings.
    ///
    TrendDisplaySettings displaySettings() const;

    ///
    /// \brief Builds one editable row per charted series.
    /// \param series Current series in display order.
    ///
    void setSeries(const QVector<TrendSeriesInfo> &series);

    ///
    /// \brief Returns the series with their edited visibility and colours.
    /// \return Updated series in the original order.
    ///
    QVector<TrendSeriesInfo> series() const;

signals:
    ///
    /// \brief Requests that a new subscription be created in the shared list.
    /// \param name New subscription name.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscriptionCreationRequested(QString name, double publishingInterval);

private:
    void selectPeriod(int mode, qint64 windowMs);
    void resetToDefaults();

    Ui::TrendSettingsDialog *ui;
    QButtonGroup *_periodGroup = nullptr;
    QStandardItemModel *_seriesModel = nullptr;
};
