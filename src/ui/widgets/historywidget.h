// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file historywidget.h
/// \brief Declares the OPC UA history tab widget.
///

#pragma once

#include <QWidget>

#include "appsettings.h"
#include "opcua/opcuatypes.h"

class QDateTime;
class QDateTimeEdit;

namespace Ui {
class HistoryWidget;
}

class HistoryModel;

///
/// \brief Tab widget for reading and exporting OPC UA history samples.
///
class HistoryWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the history widget, its toolbar and table view.
    /// \param parent Parent widget.
    ///
    explicit HistoryWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~HistoryWidget() override;

    ///
    /// \brief Shows history samples in the History table.
    /// \param values History samples in time order.
    ///
    void setHistoryResults(const QVector<OpcUaHistoryValue> &values);

    ///
    /// \brief Targets a node and requests its history for the current range.
    /// \param nodeId Node whose history should be read.
    /// \param displayName Human-readable node name.
    /// \param displayPath Human-readable path shown in the node field.
    ///
    void requestHistoryForNode(const QString &nodeId, const QString &displayName,
                               const QString &displayPath = {});

    ///
    /// \brief Builds the default CSV export file name for the current history query.
    /// \return Suggested CSV file name.
    ///
    QString suggestedHistoryCsvFileName() const;

    ///
    /// \brief Clears the selected node and its displayed samples.
    ///
    void clear();

    ///
    /// \brief Persists the history table header state.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the history table header state.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

public slots:
    ///
    /// \brief Aligns the history date pickers with the local/UTC timestamp mode.
    /// \param mode Local time or UTC.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

signals:
    ///
    /// \brief Emitted when the user requests a raw history read for a node.
    /// \param nodeId Node whose history should be read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param maxValues Maximum samples to return, or 0 for no limit.
    ///
    void historyReadRequested(QString nodeId, QDateTime start, QDateTime end, quint32 maxValues);

private:
    void setupHistoryView();
    void updateActionButtons();
    void clearHistoryNode();
    void requestHistoryRead();
    void exportHistoryToCsv();
    void applyHistoryTimestampMode(AppSettings::TimestampMode mode);
    void updateHistoryZoneSuffix(QDateTimeEdit *edit);

    Ui::HistoryWidget *ui;
    HistoryModel      *_historyModel;
};
