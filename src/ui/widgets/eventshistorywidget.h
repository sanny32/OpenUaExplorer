// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file eventshistorywidget.h
/// \brief Declares the OPC UA events history tab widget.
///

#pragma once

#include <QWidget>

#include "appsettings.h"
#include "opcua/opcuatypes.h"

class QDateTime;
class QDateTimeEdit;

namespace Ui {
class EventsHistoryWidget;
}

class EventsModel;

///
/// \brief Tab widget for reading and exporting OPC UA historical events.
///
class EventsHistoryWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the events history widget, its toolbar and table view.
    /// \param parent Parent widget.
    ///
    explicit EventsHistoryWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~EventsHistoryWidget() override;

    ///
    /// \brief Shows historical events in the table.
    /// \param events Events to display.
    ///
    void setEventsHistoryResults(const QVector<OpcUaEvent> &events);

    ///
    /// \brief Targets a node and requests its historical events for the current range.
    /// \param nodeId Node whose event history should be read.
    /// \param displayName Human-readable node name.
    /// \param displayPath Human-readable path shown in the node field.
    ///
    void requestEventsHistoryForNode(const QString &nodeId, const QString &displayName,
                                     const QString &displayPath = {});

    ///
    /// \brief Builds the default CSV export file name for the current event-history query.
    /// \return Suggested CSV file name.
    ///
    QString suggestedEventsHistoryCsvFileName() const;

    ///
    /// \brief Clears the selected node and displayed events.
    ///
    void clear();

    ///
    /// \brief Prompts for a file and exports the displayed event history as CSV.
    ///
    void exportEventsHistoryToCsv();

    ///
    /// \brief Persists the events history table header state.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the events history table header state.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

public slots:
    ///
    /// \brief Aligns the event-history date pickers with the local/UTC timestamp mode.
    /// \param mode Local time or UTC.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

signals:
    ///
    /// \brief Emitted when the user requests an event-history read for a node.
    /// \param nodeId Node whose event history should be read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param maxValues Maximum events to return, or 0 for no limit.
    ///
    void eventsHistoryReadRequested(QString nodeId, QDateTime start, QDateTime end,
                                    quint32 maxValues);

protected:
    void changeEvent(QEvent *event) override;

private:
    void setupEventsHistoryView();
    void updateActionButtons();
    void clearEventsHistoryNode();
    void requestEventsHistoryRead();
    void applyEventsHistoryTimestampMode(AppSettings::TimestampMode mode);
    void updateEventsHistoryZoneSuffix(QDateTimeEdit *edit);

    Ui::EventsHistoryWidget *ui;
    EventsModel             *_eventsHistoryModel;
};
