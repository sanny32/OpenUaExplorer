// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file writevaluedialog.h
/// \brief Declares the typed OPC UA value editor dialog.
///

#pragma once

#include <QDialog>
#include <QVariant>

namespace Ui {
class WriteValueDialog;
}

///
/// \brief Validates and converts a value before writing it to an OPC UA node.
///
class WriteValueDialog : public QDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog, fills the type list, and wires the button box.
    /// \param parent Parent widget.
    ///
    explicit WriteValueDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~WriteValueDialog() override;

    ///
    /// \brief Seeds the editor from the current value and locks it when not writable.
    /// \param value Current value.
    /// \param valueType QOpcUa::Types numeric value.
    /// \param dataTypeId OPC UA DataType NodeId.
    /// \param writable Whether the server permits writing.
    ///
    void setValue(const QVariant &value, int valueType,
                  const QString &dataTypeId, bool writable);

    ///
    /// \brief Returns the value entered by the user.
    /// \return Converted value.
    ///
    QVariant value() const;

    ///
    /// \brief Returns the selected OPC UA value type.
    /// \return Selected QOpcUa::Types numeric value.
    ///
    int valueType() const;

private slots:
    void validateAndAccept();

private:
    QVariant convertScalar(const QString &text, int type, bool *ok) const;
    void populateTypes();

    Ui::WriteValueDialog *ui;
    QVariant _value;
};
