// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeswidget.cpp
/// \brief Implements the selected node attributes widget.
///

#include <QDateTime>
#include <QHeaderView>
#include <QMessageBox>
#include <QVector>

#include "appsettings.h"
#include "attributeswidget.h"
#include "opcua/attributeformatter.h"
#include "models/attributesmodel.h"
#include "ui_attributeswidget.h"

namespace {

///
/// \brief QOpcUa::Types numeric values for the editable scalar types.
///
enum OpcUaType {
    Boolean = 0, Int32 = 1, UInt32 = 2, Double = 3, Float = 4,
    String = 5, LocalizedText = 6, DateTime = 7, UInt16 = 8, Int16 = 9,
    UInt64 = 10, Int64 = 11, Byte = 12, SByte = 13, ByteString = 14,
    XmlElement = 15, NodeId = 16, Guid = 17, QualifiedName = 18,
    StatusCode = 19, ExpandedNodeId = 27,
};

///
/// \brief Returns the display name for an OPC UA value type.
///
QString typeName(int type)
{
    switch (type) {
    case Boolean:        return QStringLiteral("Boolean");
    case SByte:          return QStringLiteral("SByte");
    case Byte:           return QStringLiteral("Byte");
    case Int16:          return QStringLiteral("Int16");
    case UInt16:         return QStringLiteral("UInt16");
    case Int32:          return QStringLiteral("Int32");
    case UInt32:         return QStringLiteral("UInt32");
    case Int64:          return QStringLiteral("Int64");
    case UInt64:         return QStringLiteral("UInt64");
    case Float:          return QStringLiteral("Float");
    case Double:         return QStringLiteral("Double");
    case String:         return QStringLiteral("String");
    case LocalizedText:  return QStringLiteral("LocalizedText");
    case XmlElement:     return QStringLiteral("XmlElement");
    case QualifiedName:  return QStringLiteral("QualifiedName");
    case DateTime:       return QStringLiteral("DateTime");
    case Guid:           return QStringLiteral("Guid");
    case ByteString:     return QStringLiteral("ByteString");
    case NodeId:         return QStringLiteral("NodeId");
    case ExpandedNodeId: return QStringLiteral("ExpandedNodeId");
    case StatusCode:     return QStringLiteral("StatusCode");
    default:             return QStringLiteral("Unknown");
    }
}

///
/// \brief Returns the value types interchangeable with the given node type.
///
/// Numeric, textual, and NodeId-like types are grouped so the user can pick a
/// concrete type within the same family (e.g. when the node's DataType is an
/// abstract Number). Other types stand alone.
///
QVector<int> compatibleTypes(int type)
{
    static const QVector<int> numeric = {SByte, Byte, Int16, UInt16, Int32,
                                         UInt32, Int64, UInt64, Float, Double};
    static const QVector<int> text = {String, LocalizedText, XmlElement, QualifiedName};
    static const QVector<int> nodeIds = {NodeId, ExpandedNodeId};
    if (numeric.contains(type))
        return numeric;
    if (text.contains(type))
        return text;
    if (nodeIds.contains(type))
        return nodeIds;
    return {type};
}

///
/// \brief Returns a sensible default value text for an OPC UA value type.
///
QString defaultValueText(int type)
{
    switch (type) {
    case Boolean:
        return QStringLiteral("false");
    case Float:
    case Double:
        return QStringLiteral("0.0");
    case DateTime:
        return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    case Guid:
        return QStringLiteral("00000000-0000-0000-0000-000000000000");
    case SByte:
    case Byte:
    case Int16:
    case UInt16:
    case Int32:
    case UInt32:
    case Int64:
    case UInt64:
    case StatusCode:
        return QStringLiteral("0");
    default:
        return QString();
    }
}

} // namespace

///
/// \brief Builds the attributes widget and its tree view.
/// \param parent Parent widget.
///
AttributesWidget::AttributesWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AttributesWidget)
    , _model(new AttributesModel(this))
{
    ui->setupUi(this);
    setupAttributesView();
    connect(ui->writeButton, &QPushButton::clicked,
            this, &AttributesWidget::writeCurrentValue);
}

///
/// \brief Destroys the widget and its generated UI.
///
AttributesWidget::~AttributesWidget()
{
    delete ui;
}

///
/// \brief Shows the attributes of the selected node, expanding the top level.
/// \param details Selected node details.
///
void AttributesWidget::setNodeDetails(const OpcUaNodeDetails &details)
{
    _model->setAttributes(details.attributes);
    ui->attributesTree->expandToDepth(0);

    _nodeId = details.nodeId;
    const bool variable = OpcUa::isVariable(details.nodeClass);
    const bool writable = variable && OpcUa::isWritable(details.userAccessLevel);
    ui->writeValueGroup->setEnabled(writable);
    if (variable)
        setupWriteEditor(details.valueType);
    else
        clearWriteEditor();
}

///
/// \brief Clears the attributes view.
///
void AttributesWidget::clear()
{
    _model->clear();
    ui->writeValueGroup->setEnabled(false);
    clearWriteEditor();
}

///
/// \brief Persists the attributes tree header state.
/// \param settings Settings store to write to.
///
void AttributesWidget::saveViewState(AppSettings &settings) const
{
    settings.setViewState(ui->attributesTree->objectName(),
                          ui->attributesTree->header()->saveState());
}

///
/// \brief Restores the attributes tree header state.
/// \param settings Settings store to read from.
///
void AttributesWidget::restoreViewState(AppSettings &settings)
{
    const QByteArray state = settings.viewState(ui->attributesTree->objectName());
    if (!state.isEmpty())
        ui->attributesTree->header()->restoreState(state);
}

///
/// \brief Binds the tree to the attributes model and configures its columns.
///
void AttributesWidget::setupAttributesView()
{
    ui->attributesTree->setModel(_model);
    auto *header = ui->attributesTree->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(AttributesModel::ColAttribute, QHeaderView::Fixed);
    header->setSectionResizeMode(AttributesModel::ColValue, QHeaderView::Stretch);

    ui->attributesTree->setColumnWidth(AttributesModel::ColAttribute, 165);

    ui->writeValueGroup->setEnabled(false);
    clearWriteEditor();
}

///
/// \brief Fills the type combo with types compatible with the node and seeds a default value.
/// \param valueType QOpcUa::Types numeric value of the selected node.
///
void AttributesWidget::setupWriteEditor(int valueType)
{
    ui->typeCombo->clear();
    const QVector<int> types = compatibleTypes(valueType);
    for (int type : types)
        ui->typeCombo->addItem(typeName(type), type);

    const int index = ui->typeCombo->findData(valueType);
    ui->typeCombo->setCurrentIndex(index >= 0 ? index : 0);
    ui->valueEdit->setDefaultValue(defaultValueText(valueType));
}

///
/// \brief Empties the type combo and the value field.
///
void AttributesWidget::clearWriteEditor()
{
    _nodeId.clear();
    ui->typeCombo->clear();
    ui->valueEdit->setDefaultValue(QString());
}

///
/// \brief Converts the entered text to the selected type and requests the write.
///
void AttributesWidget::writeCurrentValue()
{
    if (_nodeId.isEmpty() || ui->typeCombo->currentIndex() < 0)
        return;

    const int valueType = ui->typeCombo->currentData().toInt();
    bool ok = false;
    const QVariant value = OpcUaFormat::scalarFromText(
        ui->valueEdit->text(), static_cast<QOpcUa::Types>(valueType), &ok);
    if (!ok) {
        QMessageBox::warning(this, tr("Invalid Value"),
                             tr("The value is invalid or outside the selected type range."));
        return;
    }
    emit writeRequested(_nodeId, value, valueType);
}
