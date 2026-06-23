// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file writevaluedialog.cpp
/// \brief Implements the typed OPC UA value editor dialog.
///

#include <limits>

#include <QDateTime>
#include <QDialogButtonBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMessageBox>
#include <QPushButton>
#include <QUuid>

#include "opcua/attributeformatter.h"
#include "ui_writevaluedialog.h"
#include "writevaluedialog.h"

namespace {
enum OpcUaType {
    Boolean = 0,
    Int32 = 1,
    UInt32 = 2,
    Double = 3,
    Float = 4,
    String = 5,
    LocalizedText = 6,
    DateTime = 7,
    UInt16 = 8,
    Int16 = 9,
    UInt64 = 10,
    Int64 = 11,
    Byte = 12,
    SByte = 13,
    ByteString = 14,
    XmlElement = 15,
    NodeId = 16,
    Guid = 17,
    QualifiedName = 18,
    StatusCode = 19,
    ExtensionObject = 20,
    ExpandedNodeId = 27,
    Undefined = -1
};
}

///
/// \brief Builds the dialog, fills the type list, and wires the button box.
/// \param parent Parent widget.
///
WriteValueDialog::WriteValueDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::WriteValueDialog)
{
    ui->setupUi(this);
    populateTypes();
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &WriteValueDialog::validateAndAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
}

///
/// \brief Destroys the dialog and its generated UI.
///
WriteValueDialog::~WriteValueDialog()
{
    delete ui;
}

///
/// \brief Seeds the editor from the current value and locks it when not writable.
/// \param value Current value.
/// \param valueType QOpcUa::Types numeric value.
/// \param dataTypeId OPC UA DataType NodeId.
/// \param writable Whether the server permits writing.
///
void WriteValueDialog::setValue(const QVariant &value, int valueType,
                                const QString &dataTypeId, bool writable)
{
    ui->dataTypeLabel->setText(OpcUaFormat::dataTypeDisplay(dataTypeId));
    const int index = ui->typeComboBox->findData(valueType);
    ui->typeComboBox->setCurrentIndex(index >= 0 ? index : ui->typeComboBox->findData(Undefined));

    const bool array = value.canConvert<QVariantList>()
        && value.userType() != QMetaType::QString
        && value.userType() != QMetaType::QByteArray;
    ui->arrayCheckBox->setChecked(array);
    if (array || value.userType() == QMetaType::QVariantMap) {
        ui->valueEdit->setPlainText(
            QString::fromUtf8(QJsonDocument::fromVariant(value).toJson(QJsonDocument::Indented)));
    } else if (value.userType() == QMetaType::QDateTime) {
        ui->valueEdit->setPlainText(value.toDateTime().toString(Qt::ISODateWithMs));
    } else if (value.userType() == QMetaType::QByteArray) {
        ui->valueEdit->setPlainText(QString::fromLatin1(value.toByteArray().toBase64()));
    } else {
        ui->valueEdit->setPlainText(value.toString());
    }

    const bool knownExtension = value.userType() != QMetaType::QByteArray
        && value.userType() != QMetaType::QVariantMap;
    const bool editable = writable
        && (valueType != ExtensionObject || knownExtension);
    ui->valueEdit->setReadOnly(!editable);
    ui->typeComboBox->setEnabled(editable);
    ui->arrayCheckBox->setEnabled(editable);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(editable);
    ui->messageLabel->setText(editable
        ? QString()
        : tr("This value is read-only or its ExtensionObject schema is unknown."));
}

///
/// \brief Returns the value entered by the user.
/// \return Converted value.
///
QVariant WriteValueDialog::value() const
{
    return _value;
}

///
/// \brief Returns the selected OPC UA value type.
/// \return Selected QOpcUa::Types numeric value.
///
int WriteValueDialog::valueType() const
{
    return ui->typeComboBox->currentData().toInt();
}

///
/// \brief Parses the input (scalar or JSON array), and accepts only when valid.
///
void WriteValueDialog::validateAndAccept()
{
    const QString text = ui->valueEdit->toPlainText();
    bool ok = false;
    if (ui->arrayCheckBox->isChecked()) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
            QMessageBox::warning(this, tr("Invalid Value"),
                                 tr("Array values must be entered as a JSON array."));
            return;
        }
        QVariantList converted;
        for (const QVariant &entry : document.array().toVariantList()) {
            const QVariant item = convertScalar(entry.toString(), valueType(), &ok);
            if (!ok) {
                QMessageBox::warning(this, tr("Invalid Value"),
                                     tr("An array element is invalid for the selected type."));
                return;
            }
            converted.append(item);
        }
        _value = converted;
    } else {
        _value = convertScalar(text, valueType(), &ok);
        if (!ok) {
            QMessageBox::warning(this, tr("Invalid Value"),
                                 tr("The value is invalid or outside the selected type range."));
            return;
        }
    }
    accept();
}

///
/// \brief Converts text to a typed scalar, range-checking integral types.
/// \param text Source text.
/// \param type QOpcUa::Types numeric value.
/// \param ok Receives conversion status.
/// \return Converted scalar, or an invalid variant on failure.
///
QVariant WriteValueDialog::convertScalar(const QString &text, int type, bool *ok) const
{
    return OpcUaFormat::scalarFromText(text, static_cast<QOpcUa::Types>(type), ok);
}

///
/// \brief Fills the type combo box with the supported OPC UA types.
///
void WriteValueDialog::populateTypes()
{
    const QList<QPair<QString, int>> types = {
        {tr("Boolean"), Boolean}, {tr("SByte"), SByte}, {tr("Byte"), Byte},
        {tr("Int16"), Int16}, {tr("UInt16"), UInt16}, {tr("Int32"), Int32},
        {tr("UInt32"), UInt32}, {tr("Int64"), Int64}, {tr("UInt64"), UInt64},
        {tr("Float"), Float}, {tr("Double"), Double}, {tr("String"), String},
        {tr("DateTime"), DateTime}, {tr("Guid"), Guid}, {tr("ByteString"), ByteString},
        {tr("XmlElement"), XmlElement}, {tr("NodeId"), NodeId},
        {tr("ExpandedNodeId"), ExpandedNodeId}, {tr("LocalizedText"), LocalizedText},
        {tr("QualifiedName"), QualifiedName}, {tr("StatusCode"), StatusCode},
        {tr("ExtensionObject"), ExtensionObject}, {tr("Auto"), Undefined}
    };
    for (const auto &type : types)
        ui->typeComboBox->addItem(type.first, type.second);
}
