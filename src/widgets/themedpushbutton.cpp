#include <QEvent>

#include "appicons.h"
#include "themedpushbutton.h"

ThemedPushButton::ThemedPushButton(QWidget *parent)
    : QPushButton(parent)
{
}

void ThemedPushButton::setThemedIcon(const QString &name)
{
    _iconName = name;
    refreshIcon();
}

void ThemedPushButton::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);

    if (!_iconName.isEmpty()
        && (event->type() == QEvent::PaletteChange
            || event->type() == QEvent::ApplicationPaletteChange)) {
        refreshIcon();
    }
}

void ThemedPushButton::refreshIcon()
{
    setIcon(AppIcons::themed(_iconName));
}
