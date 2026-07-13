// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file callmethoddialog.cpp
/// \brief Implements the OPC UA method-call dialog.
///

#include <QHeaderView>
#include <QPushButton>
#include <QSize>

#include "appcolors.h"
#include "callmethoddialog.h"
#include "formatters/attributeformatter.h"
#include "messageboxdialog.h"
#include "opcua/opcuabackend.h"
#include "ui_callmethoddialog.h"
#include "widgets/tableview.h"

namespace {

/// \brief Default width hint for the Value column, so it is comfortable to type in.
constexpr int kValueColumnHintWidth = 160;

} // namespace

///
/// \brief Constructs a model whose Value column is editable when requested.
/// \param editableValues Whether the Value column accepts user input.
/// \param parent Owning QObject.
///
MethodArgumentModel::MethodArgumentModel(bool editableValues, QObject *parent)
    : QAbstractTableModel(parent)
    , _editableValues(editableValues)
{
}

///
/// \brief Replaces the model rows with the given arguments, clearing any values.
/// \param arguments Arguments to display.
///
void MethodArgumentModel::setArguments(const QVector<OpcUaMethodArgument> &arguments)
{
    beginResetModel();
    _rows.clear();
    _rows.reserve(arguments.size());
    for (const OpcUaMethodArgument &argument : arguments) {
        Row row;
        row.name = argument.name;
        row.dataType = OpcUaFormat::dataTypeDisplay(argument.dataTypeId);
        row.description = argument.description;
        _rows.append(row);
    }
    endResetModel();
}

///
/// \brief Returns the current Value text of a row.
/// \param row Row index.
/// \return Value text, or empty when out of range.
///
QString MethodArgumentModel::valueText(int row) const
{
    return row >= 0 && row < _rows.size() ? _rows.at(row).value : QString();
}

///
/// \brief Sets the Value text of a row.
/// \param row Row index.
/// \param text New value text.
///
void MethodArgumentModel::setValueText(int row, const QString &text)
{
    if (row < 0 || row >= _rows.size())
        return;
    _rows[row].value = text;
    const QModelIndex changed = index(row, ValueColumn);
    emit dataChanged(changed, changed, { Qt::DisplayRole, Qt::EditRole });
}

///
/// \brief Clears the Value text of every row.
///
void MethodArgumentModel::clearValues()
{
    for (int row = 0; row < _rows.size(); ++row)
        _rows[row].value.clear();
    if (!_rows.isEmpty())
        emit dataChanged(index(0, ValueColumn), index(_rows.size() - 1, ValueColumn),
                         { Qt::DisplayRole, Qt::EditRole });
}

///
/// \brief Returns the number of argument rows.
/// \param parent Unused parent index.
/// \return Row count.
///
int MethodArgumentModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _rows.size();
}

///
/// \brief Returns the fixed column count.
/// \param parent Unused parent index.
/// \return Column count.
///
int MethodArgumentModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

///
/// \brief Returns the display or edit value for a cell.
/// \param index Cell index.
/// \param role Requested role.
/// \return Cell data, or an invalid variant.
///
QVariant MethodArgumentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= _rows.size())
        return {};
    if (role == Qt::SizeHintRole && index.column() == ValueColumn)
        return QSize(kValueColumnHintWidth, 0);
    const Row &row = _rows.at(index.row());
    if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole) {
        switch (index.column()) {
        case NameColumn:        return row.name;
        case ValueColumn:       return row.value;
        case DataTypeColumn:    return row.dataType;
        case DescriptionColumn: return row.description;
        default:                return {};
        }
    }
    return {};
}

///
/// \brief Stores edited Value text.
/// \param index Cell index.
/// \param value New value.
/// \param role Edit role.
/// \return True when the value was stored.
///
bool MethodArgumentModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || !index.isValid() || index.column() != ValueColumn
        || index.row() >= _rows.size() || !_editableValues)
        return false;
    _rows[index.row()].value = value.toString();
    emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
    return true;
}

///
/// \brief Returns the column header labels.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Requested role.
/// \return Header text, or an invalid variant.
///
QVariant MethodArgumentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case NameColumn:        return tr("Name");
    case ValueColumn:       return tr("Value");
    case DataTypeColumn:    return tr("DataType");
    case DescriptionColumn: return tr("Description");
    default:                return {};
    }
}

///
/// \brief Returns the item flags, making only the input Value column editable.
/// \param index Cell index.
/// \return Item flags.
///
Qt::ItemFlags MethodArgumentModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags itemFlags = QAbstractTableModel::flags(index);
    if (_editableValues && index.column() == ValueColumn)
        itemFlags |= Qt::ItemIsEditable;
    else
        itemFlags &= ~Qt::ItemIsEditable;
    return itemFlags;
}

namespace {

///
/// \brief Applies the shared configuration to an argument view.
/// \param view Table view to configure.
///
/// Column sizing is delegated to TableView's full-text horizontal scrolling: every
/// column is auto-sized to its full content and a horizontal scrollbar appears when
/// the columns exceed the viewport, so no cell text is elided or wrapped.
///
void setupArgumentView(TableView *view)
{
    view->verticalHeader()->setVisible(false);
    view->setSelectionMode(QAbstractItemView::NoSelection);
}

} // namespace

///
/// \brief Builds the dialog and wires it to the backend.
/// \param service OPC UA backend used to read metadata and call the method.
/// \param parent Parent widget.
///
CallMethodDialog::CallMethodDialog(OpcUaBackend *service, QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::CallMethodDialog)
    , _service(service)
    , _inputModel(new MethodArgumentModel(true, this))
    , _outputModel(new MethodArgumentModel(false, this))
{
    ui->setupUi(this);

    ui->inputTable->setModel(_inputModel);
    ui->outputTable->setModel(_outputModel);
    setupArgumentView(ui->inputTable);
    setupArgumentView(ui->outputTable);
    ui->outputTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    _callButton = ui->buttonBox->button(QDialogButtonBox::Apply);
    if (_callButton)
        _callButton->setText(tr("Call"));

    connect(_callButton, &QPushButton::clicked, this, &CallMethodDialog::callMethod);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);

    connect(_service, &OpcUaBackend::methodInfoReady,
            this, &CallMethodDialog::handleMethodInfo);
    connect(_service, &OpcUaBackend::methodCallFinished,
            this, &CallMethodDialog::handleMethodCall);

    setCallEnabled(false);
}

///
/// \brief Destroys the dialog and its generated UI.
///
CallMethodDialog::~CallMethodDialog()
{
    delete ui;
}

///
/// \brief Points the dialog at a method and its owning object, reading argument metadata.
/// \param object Object node that owns the method.
/// \param method Method node to call.
///
void CallMethodDialog::setTarget(const OpcUaNodeInfo &object, const OpcUaNodeInfo &method)
{
    _object = object;
    _method = method;

    const QString methodName = method.displayName.isEmpty() ? method.browseName : method.displayName;
    const QString objectName = object.displayName.isEmpty() ? object.browseName : object.displayName;
    setWindowTitle(tr("Call %1 on %2").arg(methodName, objectName));

    populateInputs({});
    populateOutputs({});
    setResultText(QString(), true);
    setCallEnabled(false);

    if (!_method.nodeId.isEmpty())
        _service->readMethodInfo(_method.nodeId);
}

///
/// \brief Returns the NodeId of the targeted method, or empty when none is set.
/// \return Method NodeId.
///
QString CallMethodDialog::methodNodeId() const
{
    return _method.nodeId;
}

///
/// \brief Populates the argument tables once the method's metadata has been read.
/// \param methodNodeId Method whose metadata was read.
/// \param inputs Input argument descriptions.
/// \param outputs Output argument descriptions.
/// \param error Read error, empty on success.
///
void CallMethodDialog::handleMethodInfo(const QString &methodNodeId,
                                        const QVector<OpcUaMethodArgument> &inputs,
                                        const QVector<OpcUaMethodArgument> &outputs,
                                        const QString &error)
{
    if (methodNodeId != _method.nodeId)
        return;
    populateInputs(inputs);
    populateOutputs(outputs);
    if (!error.isEmpty())
        setResultText(error, false);
    setCallEnabled(true);
}

///
/// \brief Fills the output values and status when a call finishes.
/// \param methodNodeId Called method.
/// \param result Raw output value returned by the server.
/// \param success Whether the call succeeded.
/// \param error Call error, empty on success.
///
void CallMethodDialog::handleMethodCall(const QString &methodNodeId, const QVariant &result,
                                        bool success, const QString &error)
{
    if (methodNodeId != _method.nodeId)
        return;
    setCallEnabled(true);
    if (success) {
        setOutputValues(result);
        setResultText(tr("Good"), true);
    } else {
        clearOutputValues();
        setResultText(error, false);
    }
}

///
/// \brief Converts the input rows to typed values and calls the method.
///
void CallMethodDialog::callMethod()
{
    QVariantList values;
    QList<int> types;
    for (int row = 0; row < _inputs.size(); ++row) {
        const OpcUaMethodArgument &argument = _inputs.at(row);
        const QString text = _inputModel->valueText(row);
        bool ok = false;
        const QVariant value = OpcUaFormat::scalarFromText(
            text, static_cast<QOpcUa::Types>(argument.valueType), &ok);
        if (!ok) {
            MessageBoxDialog::warning(this, tr("Invalid Argument"),
                tr("The value of '%1' is invalid for its data type.").arg(argument.name),
                DialogButtonBox::Ok);
            return;
        }
        values.append(value);
        types.append(argument.valueType);
    }

    clearOutputValues();
    setResultText(tr("Calling…"), true);
    setCallEnabled(false);
    _service->callMethod(_object.nodeId, _method.nodeId, values, types);
}

///
/// \brief Rebuilds the input table from argument metadata.
/// \param inputs Input argument descriptions.
///
void CallMethodDialog::populateInputs(const QVector<OpcUaMethodArgument> &inputs)
{
    _inputs = inputs;
    _inputModel->setArguments(inputs);
    ui->inputGroup->setVisible(!inputs.isEmpty());
}

///
/// \brief Rebuilds the output table from argument metadata.
/// \param outputs Output argument descriptions.
///
void CallMethodDialog::populateOutputs(const QVector<OpcUaMethodArgument> &outputs)
{
    _outputs = outputs;
    _outputModel->setArguments(outputs);
    ui->outputGroup->setVisible(!outputs.isEmpty());
}

///
/// \brief Writes returned values into the output rows.
/// \param result Raw output value: a single value, or a list for several outputs.
///
void CallMethodDialog::setOutputValues(const QVariant &result)
{
    if (_outputs.isEmpty())
        return;
    if (_outputs.size() == 1) {
        _outputModel->setValueText(0, OpcUaFormat::displayValue(result));
        return;
    }
    const QVariantList list = result.toList();
    for (int row = 0; row < _outputs.size(); ++row)
        _outputModel->setValueText(
            row, row < list.size() ? OpcUaFormat::displayValue(list.at(row)) : QString());
}

///
/// \brief Clears the value column of every output row.
///
void CallMethodDialog::clearOutputValues()
{
    _outputModel->clearValues();
}

///
/// \brief Sets the result label text and colour.
/// \param text Status text.
/// \param success Whether the status represents success.
///
void CallMethodDialog::setResultText(const QString &text, bool success)
{
    _resultText = text;
    _resultSuccess = success;
    ui->resultLabel->setText(text);
    applyResultColor();
}

///
/// \brief Applies the success/error colour to the current result text.
///
void CallMethodDialog::applyResultColor()
{
    const QColor color = _resultSuccess ? AppColors::statusSuccess() : AppColors::statusError();
    ui->resultLabel->setStyleSheet(QStringLiteral("color:%1;").arg(color.name()));
}

///
/// \brief Enables or disables the Call button.
/// \param enabled Whether calling is allowed.
///
void CallMethodDialog::setCallEnabled(bool enabled)
{
    if (_callButton)
        _callButton->setEnabled(enabled);
}

///
/// \brief Re-applies the result colour after the palette has switched.
/// \param event Change event.
///
void CallMethodDialog::changeEvent(QEvent *event)
{
    AppBaseDialog::changeEvent(event);
    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange) {
        applyResultColor();
    }
}
