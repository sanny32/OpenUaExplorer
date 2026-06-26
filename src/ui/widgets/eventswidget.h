// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file eventswidget.h
/// \brief Declares the OPC UA events tab widget.
///

#pragma once

#include <QWidget>

#include "appsettings.h"

namespace Ui {
class EventsWidget;
}

class EventsModel;

///
/// \brief Tab widget that displays OPC UA event entries.
///
class EventsWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the events widget and its table view.
    /// \param parent Parent widget.
    ///
    explicit EventsWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~EventsWidget() override;

    ///
    /// \brief Removes all displayed events.
    ///
    void clear();

    ///
    /// \brief Persists the events table header state.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the events table header state.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

private:
    void setupEventsView();

    Ui::EventsWidget *ui;
    EventsModel      *_eventsModel;
};
