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
    explicit WriteValueDialog(QWidget *parent = nullptr);
    ~WriteValueDialog() override;

    void setValue(const QVariant &value, int valueType,
                  const QString &dataTypeId, bool writable);
    QVariant value() const;
    int valueType() const;

private slots:
    void validateAndAccept();

private:
    QVariant convertScalar(const QString &text, int type, bool *ok) const;
    void populateTypes();

    Ui::WriteValueDialog *ui;
    QVariant _value;
};
