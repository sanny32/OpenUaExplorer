#include <QEvent>

#include "appicons.h"
#include "themediconlabel.h"

///
/// \brief ThemedIconLabel::ThemedIconLabel
/// \param parent
///
ThemedIconLabel::ThemedIconLabel(QWidget *parent)
    : QLabel(parent)
{
}

///
/// \brief ThemedIconLabel::setIcon
/// \param name
/// \param size
///
void ThemedIconLabel::setIcon(const QString &name, int size)
{
    _iconName = name;
    _size     = size;
    refreshIcon();
}

///
/// \brief ThemedIconLabel::changeEvent
/// \param event
///
void ThemedIconLabel::changeEvent(QEvent *event)
{
    QLabel::changeEvent(event);

    if (!_iconName.isEmpty()
        && (event->type() == QEvent::PaletteChange
            || event->type() == QEvent::ApplicationPaletteChange)) {
        refreshIcon();
    }
}

///
/// \brief ThemedIconLabel::refreshIcon
///
void ThemedIconLabel::refreshIcon()
{
    setPixmap(AppIcons::themed(_iconName).pixmap(_size, _size));
}
