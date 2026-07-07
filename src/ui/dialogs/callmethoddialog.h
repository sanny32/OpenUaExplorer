// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file callmethoddialog.h
/// \brief Declares the OPC UA method-call dialog.
///

#pragma once

#include <QAbstractTableModel>
#include <QString>
#include <QVariant>
#include <QVector>

#include "dialogs/appbasedialog.h"
#include "opcua/opcuatypes.h"

namespace Ui {
class CallMethodDialog;
}

class OpcUaClientService;
class QPushButton;

///
/// \brief Table model backing the input/output argument views of the call dialog.
///
/// Presents one row per method argument with Name, Value, DataType and Description
/// columns. The Value column is editable only in input mode; the Description column
/// carries the full text so a word-wrapping view can grow the row to fit it.
///
class MethodArgumentModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /// \brief Column layout shared by both argument tables.
    enum Column {
        NameColumn = 0,
        ValueColumn = 1,
        DataTypeColumn = 2,
        DescriptionColumn = 3,
        ColumnCount = 4
    };

    ///
    /// \brief Constructs a model whose Value column is editable when requested.
    /// \param editableValues Whether the Value column accepts user input.
    /// \param parent Owning QObject.
    ///
    explicit MethodArgumentModel(bool editableValues, QObject *parent = nullptr);

    ///
    /// \brief Replaces the model rows with the given arguments, clearing any values.
    /// \param arguments Arguments to display.
    ///
    void setArguments(const QVector<OpcUaMethodArgument> &arguments);

    ///
    /// \brief Returns the current Value text of a row.
    /// \param row Row index.
    /// \return Value text, or empty when out of range.
    ///
    QString valueText(int row) const;

    ///
    /// \brief Sets the Value text of a row.
    /// \param row Row index.
    /// \param text New value text.
    ///
    void setValueText(int row, const QString &text);

    ///
    /// \brief Clears the Value text of every row.
    ///
    void clearValues();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    struct Row {
        QString name;
        QString dataType;
        QString description;
        QString value;
    };

    QVector<Row> _rows;
    bool _editableValues;
};

///
/// \brief Collects a method's input arguments, calls it, and shows the outputs.
///
/// Mirrors UaExpert's method-call dialog: on retargeting it reads the method's
/// InputArguments/OutputArguments metadata, builds a typed input row per argument
/// and an output row per result, then invokes the method on its owning object and
/// fills in the returned values plus a status. The dialog stays open after a call so
/// the user can adjust inputs and call again.
///
class CallMethodDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog and wires it to the client service.
    /// \param service OPC UA client service used to read metadata and call the method.
    /// \param parent Parent widget.
    ///
    explicit CallMethodDialog(OpcUaClientService *service, QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~CallMethodDialog() override;

    ///
    /// \brief Points the dialog at a method and its owning object, reading argument metadata.
    /// \param object Object node that owns the method.
    /// \param method Method node to call.
    ///
    void setTarget(const OpcUaNodeInfo &object, const OpcUaNodeInfo &method);

    ///
    /// \brief Returns the NodeId of the targeted method, or empty when none is set.
    /// \return Method NodeId.
    ///
    QString methodNodeId() const;

protected:
    void changeEvent(QEvent *event) override;

private:
    void handleMethodInfo(const QString &methodNodeId,
                          const QVector<OpcUaMethodArgument> &inputs,
                          const QVector<OpcUaMethodArgument> &outputs, const QString &error);
    void handleMethodCall(const QString &methodNodeId, const QVariant &result,
                          bool success, const QString &error);
    void callMethod();
    void populateInputs(const QVector<OpcUaMethodArgument> &inputs);
    void populateOutputs(const QVector<OpcUaMethodArgument> &outputs);
    void setOutputValues(const QVariant &result);
    void clearOutputValues();
    void setResultText(const QString &text, bool success);
    void applyResultColor();
    void setCallEnabled(bool enabled);

    Ui::CallMethodDialog *ui;
    OpcUaClientService *_service;
    MethodArgumentModel *_inputModel;
    MethodArgumentModel *_outputModel;
    QPushButton *_callButton = nullptr;
    OpcUaNodeInfo _object;
    OpcUaNodeInfo _method;
    QVector<OpcUaMethodArgument> _inputs;
    QVector<OpcUaMethodArgument> _outputs;
    QString _resultText;
    bool _resultSuccess = true;
};
