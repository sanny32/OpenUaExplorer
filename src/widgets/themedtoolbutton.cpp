#include <QEvent>

#include "appicons.h"
#include "themedtoolbutton.h"

ThemedToolButton::ThemedToolButton(QWidget *parent)
    : QToolButton(parent)
{
}

void ThemedToolButton::setThemedIcon(const QString &name)
{
    _iconName = name;
    refreshIcon();
}

void ThemedToolButton::changeEvent(QEvent *event)
{
    QToolButton::changeEvent(event);

    if (!_iconName.isEmpty()
        && (event->type() == QEvent::PaletteChange
            || event->type() == QEvent::ApplicationPaletteChange)) {
        refreshIcon();
    }
}

void ThemedToolButton::refreshIcon()
{
    setIcon(AppIcons::themed(_iconName));
}
