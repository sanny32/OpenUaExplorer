#include <QPalette>
#include <QStyleOptionButton>

#include "appstyle.h"

///
/// \brief AppStyle::AppStyle
/// \param style
///
AppStyle::AppStyle(QStyle *style)
    : QProxyStyle(style)
{
}

///
/// \brief AppStyle::subElementRect
/// \param element
/// \param option
/// \param widget
/// \return
///
QRect AppStyle::subElementRect(SubElement element, const QStyleOption *option,
                                const QWidget *widget) const
{
    QRect rect = QProxyStyle::subElementRect(element, option, widget);

    switch (element) {
    case SE_HeaderLabel:
        return rect.adjusted(textHMargin, textVMargin, -textHMargin, -textVMargin);
    case SE_ItemViewItemText:
        return rect.adjusted(textHMargin, 0, -textHMargin, 0);
    default:
        return rect;
    }
}
