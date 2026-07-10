// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file valuelineedit.cpp
/// \brief Implements a line edit with a reset-to-default action.
///

#include "valuelineedit.h"
#include "themedaction.h"

///
/// \brief Constructs the line edit and installs the reset action.
/// \param parent Parent widget.
///
ValueLineEdit::ValueLineEdit(QWidget *parent)
    : QLineEdit(parent)
    , _resetAction(new ThemedAction(QStringLiteral("clear"), QString(), this))
{
    _resetAction->setToolTip(tr("Reset to default value"));
    addAction(_resetAction, QLineEdit::TrailingPosition);

    connect(_resetAction, &QAction::triggered, this, [this] {
        setText(_defaultValue);
    });
    connect(this, &QLineEdit::textChanged, this, [this] {
        updateResetState();
    });
    updateResetState();
}

///
/// \brief Returns the value the reset action restores.
/// \return Current default value.
///
QString ValueLineEdit::defaultValue() const
{
    return _defaultValue;
}

///
/// \brief Sets the default value and resets the text to it.
/// \param value New default value.
///
void ValueLineEdit::setDefaultValue(const QString &value)
{
    _defaultValue = value;
    setText(value);
}

///
/// \brief Sets the tooltip shown for the reset action.
/// \param toolTip Tooltip text.
///
void ValueLineEdit::setResetToolTip(const QString &toolTip)
{
    _resetAction->setToolTip(toolTip);
}

///
/// \brief Shows the reset action only while the text differs from the default.
///
void ValueLineEdit::updateResetState()
{
    _resetAction->setVisible(text() != _defaultValue);
}
