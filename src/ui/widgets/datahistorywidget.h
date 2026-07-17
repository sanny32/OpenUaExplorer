// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file datahistorywidget.h
/// \brief Declares the OPC UA data history tab widget.
///

#pragma once

#include <QWidget>

#include "appsettings.h"
#include "opcua/opcuatypes.h"

class QDateTime;
class QDateTimeEdit;

namespace Ui {
class DataHistoryWidget;
}

class HistoryModel;

///
/// \brief Tab widget for reading and exporting OPC UA data history samples.
///
class DataHistoryWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the data history widget, its toolbar and table view.
    /// \param parent Parent widget.
    ///
    explicit DataHistoryWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~DataHistoryWidget() override;

    ///
    /// \brief Shows data history samples in the Data History table.
    /// \param values Data history samples in time order.
    ///
    void setDataHistoryResults(const QVector<OpcUaHistoryValue> &values);

    ///
    /// \brief Targets a node and requests its data history for the current range.
    /// \param nodeId Node whose data history should be read.
    /// \param displayName Human-readable node name.
    /// \param displayPath Human-readable path shown in the node field.
    ///
    void requestDataHistoryForNode(const QString &nodeId, const QString &displayName,
                               const QString &displayPath = {});

    ///
    /// \brief Builds the default CSV export file name for the current data history query.
    /// \return Suggested CSV file name.
    ///
    QString suggestedDataHistoryCsvFileName() const;

    ///
    /// \brief Clears the selected node and its displayed samples.
    ///
    void clear();

    ///
    /// \brief Prompts for a file and exports the displayed data history as CSV.
    ///
    void exportDataHistoryToCsv();

    ///
    /// \brief Persists the data history table header state.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the data history table header state.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

public slots:
    ///
    /// \brief Aligns the data history date pickers with the local/UTC timestamp mode.
    /// \param mode Local time or UTC.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

signals:
    ///
    /// \brief Emitted when the user requests a raw data history read for a node.
    /// \param nodeId Node whose data history should be read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param maxValues Maximum samples to return, or 0 for no limit.
    ///
    void dataHistoryReadRequested(QString nodeId, QDateTime start, QDateTime end, quint32 maxValues);

protected:
    void changeEvent(QEvent *event) override;

private:
    void setupDataHistoryView();
    void updateActionButtons();
    void clearDataHistoryNode();
    void requestDataHistoryRead();
    void applyDataHistoryTimestampMode(AppSettings::TimestampMode mode);
    void updateDataHistoryZoneSuffix(QDateTimeEdit *edit);

    Ui::DataHistoryWidget *ui;
    HistoryModel      *_dataHistoryModel;
};
