// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dialogbuttonbox.cpp
/// \brief Implements a dialog button box backed by ColoredPushButton instances.
///

#include <QCoreApplication>
#include <QEvent>

#include "appcolors.h"
#include "coloredpushbutton.h"
#include "dialogbuttonbox.h"

namespace {

///
/// \brief Static description of a standard button: its role and untranslated text.
///
struct StandardButtonInfo {
    QDialogButtonBox::StandardButton button;
    QDialogButtonBox::ButtonRole role;
    const char *text;
};

///
/// \brief Standard buttons in their natural creation order.
///
/// Roles and texts mirror Qt's QPlatformDialogHelper defaults so the box behaves like
/// QDialogButtonBox. Texts are looked up in the "QPlatformTheme" translation context,
/// matching the strings QDialogButtonBox itself uses.
///
constexpr StandardButtonInfo kStandardButtons[] = {
    { QDialogButtonBox::Ok, QDialogButtonBox::AcceptRole, "OK" },
    { QDialogButtonBox::Save, QDialogButtonBox::AcceptRole, "Save" },
    { QDialogButtonBox::SaveAll, QDialogButtonBox::AcceptRole, "Save All" },
    { QDialogButtonBox::Open, QDialogButtonBox::AcceptRole, "Open" },
    { QDialogButtonBox::Yes, QDialogButtonBox::YesRole, "&Yes" },
    { QDialogButtonBox::YesToAll, QDialogButtonBox::YesRole, "Yes to &All" },
    { QDialogButtonBox::No, QDialogButtonBox::NoRole, "&No" },
    { QDialogButtonBox::NoToAll, QDialogButtonBox::NoRole, "N&o to All" },
    { QDialogButtonBox::Abort, QDialogButtonBox::RejectRole, "Abort" },
    { QDialogButtonBox::Retry, QDialogButtonBox::AcceptRole, "Retry" },
    { QDialogButtonBox::Ignore, QDialogButtonBox::AcceptRole, "Ignore" },
    { QDialogButtonBox::Close, QDialogButtonBox::RejectRole, "Close" },
    { QDialogButtonBox::Cancel, QDialogButtonBox::RejectRole, "Cancel" },
    { QDialogButtonBox::Discard, QDialogButtonBox::DestructiveRole, "Discard" },
    { QDialogButtonBox::Help, QDialogButtonBox::HelpRole, "Help" },
    { QDialogButtonBox::Apply, QDialogButtonBox::ApplyRole, "Apply" },
    { QDialogButtonBox::Reset, QDialogButtonBox::ResetRole, "Reset" },
    { QDialogButtonBox::RestoreDefaults, QDialogButtonBox::ResetRole, "Restore Defaults" },
};

}

///
/// \brief Constructs a horizontal button box.
/// \param parent Parent widget.
///
DialogButtonBox::DialogButtonBox(QWidget *parent)
    : QDialogButtonBox(parent)
{
}

///
/// \brief Constructs a button box with the given orientation.
/// \param orientation Layout orientation.
/// \param parent Parent widget.
///
DialogButtonBox::DialogButtonBox(Qt::Orientation orientation, QWidget *parent)
    : QDialogButtonBox(orientation, parent)
{
}

///
/// \brief Creates the requested standard buttons as ColoredPushButton instances.
/// \param buttons Standard buttons to display.
///
void DialogButtonBox::setStandardButtons(StandardButtons buttons)
{
    QDialogButtonBox::clear();
    _standardButtons.clear();

    for (const StandardButtonInfo &info : kStandardButtons) {
        if (!(buttons & info.button))
            continue;

        auto *button = new ColoredPushButton(this);
        button->setText(QCoreApplication::translate("QPlatformTheme", info.text));
        QDialogButtonBox::addButton(button, info.role);
        _standardButtons.insert(static_cast<int>(info.button), button);

        if (info.button == QDialogButtonBox::Apply)
            button->setColors(
                { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });
    }
}

///
/// \brief Re-applies the translated standard-button text after a language change.
/// \param event Change event being handled.
///
/// The custom ColoredPushButton instances are unknown to QDialogButtonBox's own
/// retranslation, so their text is refreshed here from the same "QPlatformTheme"
/// context used when they were created.
///
void DialogButtonBox::changeEvent(QEvent *event)
{
    QDialogButtonBox::changeEvent(event);

    if (event->type() == QEvent::LanguageChange)
        retranslateButtons();
}

///
/// \brief Refreshes every present standard button's text for the active language.
///
void DialogButtonBox::retranslateButtons()
{
    for (const StandardButtonInfo &info : kStandardButtons) {
        if (ColoredPushButton *button = _standardButtons.value(static_cast<int>(info.button)))
            button->setText(QCoreApplication::translate("QPlatformTheme", info.text));
    }
}

///
/// \brief Returns the button for a standard button role.
/// \param which Standard button to look up.
/// \return Matching button, or nullptr when not present.
///
QPushButton *DialogButtonBox::button(StandardButton which) const
{
    return _standardButtons.value(static_cast<int>(which));
}

///
/// \brief Returns the standard button role backing a button instance.
/// \param button Button created by setStandardButtons().
/// \return Matching standard button, or NoButton when unknown.
///
QDialogButtonBox::StandardButton DialogButtonBox::standardButton(QAbstractButton *button) const
{
    for (auto it = _standardButtons.cbegin(); it != _standardButtons.cend(); ++it) {
        if (it.value() == button)
            return static_cast<StandardButton>(it.key());
    }
    return NoButton;
}

///
/// \brief Paints a standard button with an explicit colour set.
/// \param which Standard button to colour.
/// \param colors Light-theme colour set (the dark variant is derived).
///
void DialogButtonBox::setButtonColors(StandardButton which, const ColoredPushButton::Colors &colors)
{
    if (ColoredPushButton *button = coloredButton(which))
        button->setColors(colors);
}

///
/// \brief Restores a standard button's default palette appearance.
/// \param which Standard button to reset.
///
void DialogButtonBox::clearButtonColors(StandardButton which)
{
    if (ColoredPushButton *button = coloredButton(which))
        button->clearColors();
}

///
/// \brief Returns the coloured button backing a standard button role.
/// \param which Standard button to look up.
/// \return Matching button, or nullptr when not present.
///
ColoredPushButton *DialogButtonBox::coloredButton(StandardButton which) const
{
    return _standardButtons.value(static_cast<int>(which));
}
