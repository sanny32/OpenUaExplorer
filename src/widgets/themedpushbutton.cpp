#include <QEvent>

#include "appicons.h"
#include "themedpushbutton.h"

///
/// \brief ThemedPushButton::ThemedPushButton
/// \param parent
///
ThemedPushButton::ThemedPushButton(QWidget *parent)
    : QPushButton(parent)
{
}

///
/// \brief ThemedPushButton::setIcon
/// \param name
///
void ThemedPushButton::setIcon(const QString &name)
{
    _iconName = name;
    refreshIcon();
}

///
/// \brief ThemedPushButton::changeEvent
/// \param event
///
void ThemedPushButton::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);

    if (!_iconName.isEmpty()
        && (event->type() == QEvent::PaletteChange
            || event->type() == QEvent::ApplicationPaletteChange)) {
        refreshIcon();
    }
}

///
/// \brief ThemedPushButton::refreshIcon
///
void ThemedPushButton::refreshIcon()
{
    QPushButton::setIcon(AppIcons::themed(_iconName));
}
