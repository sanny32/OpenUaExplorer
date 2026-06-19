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
/// \brief NodeId identifiers of the abstract OPC UA DataTypes in namespace 0.
///
enum AbstractDataType {
    BaseDataType = 24, Number = 26, Integer = 27, UInteger = 28, Enumeration = 29,
};

///
/// \brief Returns the concrete value types writable to an abstract DataType node.
///
/// A Variable's DataType is fixed by the server, so for a concrete DataType only
/// that type is accepted. The combo is offered only when the node's DataType is
/// abstract, in which case any concrete subtype may be written. An empty result
/// means the DataType is concrete and the type must stay locked to the node's own.
///
QVector<int> abstractFamily(const QString &dataTypeId)
{
    bool ok = false;
    const int identifier = dataTypeId.section(QLatin1String("i="), 1).toInt(&ok);
    if (!ok || !dataTypeId.startsWith(QLatin1String("ns=0;")))
        return {};
    switch (identifier) {
    case Number:
        return {SByte, Byte, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double};
    case Integer:
        return {SByte, Int16, Int32, Int64};
    case UInteger:
        return {Byte, UInt16, UInt32, UInt64};
    case Enumeration:
        return {Int32};
    case BaseDataType:
        return {Boolean, SByte, Byte, Int16, UInt16, Int32, UInt32, Int64, UInt64,
                Float, Double, String, LocalizedText, DateTime, Guid, ByteString};
    default:
        return {};
    }
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
        setupWriteEditor(details.valueType, details.dataTypeId);
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
/// \brief Configures the type combo and seeds a default value for the node.
/// \param valueType QOpcUa::Types numeric value of the selected node.
/// \param dataTypeId DataType NodeId, used to detect abstract DataTypes.
///
/// For a concrete DataType the combo is locked to the node's own type, since the
/// server accepts no other. For an abstract DataType the combo lists the concrete
/// subtypes the user may choose between.
///
void AttributesWidget::setupWriteEditor(int valueType, const QString &dataTypeId)
{
    ui->typeCombo->clear();
    const QVector<int> family = abstractFamily(dataTypeId);
    if (family.isEmpty()) {
        ui->typeCombo->addItem(typeName(valueType), valueType);
        ui->typeCombo->setCurrentIndex(0);
        ui->typeCombo->setEnabled(false);
        ui->valueEdit->setDefaultValue(defaultValueText(valueType));
        return;
    }

    for (int type : family)
        ui->typeCombo->addItem(typeName(type), type);
    const int index = ui->typeCombo->findData(valueType);
    ui->typeCombo->setCurrentIndex(index >= 0 ? index : 0);
    ui->typeCombo->setEnabled(true);
    ui->valueEdit->setDefaultValue(defaultValueText(ui->typeCombo->currentData().toInt()));
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
