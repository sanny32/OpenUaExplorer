// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file valuelineedit.h
/// \brief Declares a line edit with a reset-to-default action.
///

#pragma once

#include <QLineEdit>

class ThemedAction;

///
/// \brief Line edit whose trailing action restores a configurable default value.
///
/// The reset action is shown only while the text differs from the default, so it
/// only offers to undo user edits rather than to blank the field.
///
class ValueLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the line edit and installs the reset action.
    /// \param parent Parent widget.
    ///
    explicit ValueLineEdit(QWidget *parent = nullptr);

    ///
    /// \brief Returns the value the reset action restores.
    /// \return Current default value.
    ///
    QString defaultValue() const;

    ///
    /// \brief Sets the default value and resets the text to it.
    /// \param value New default value.
    ///
    void setDefaultValue(const QString &value);

private:
    void updateResetState();

    ThemedAction *_resetAction;
    QString       _defaultValue;
};
