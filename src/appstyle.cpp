#include <QPainter>
#include <QPalette>
#include <QStyleOptionButton>
#include <QStyleOptionToolButton>

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
/// \brief AppStyle::drawControl
/// \param element
/// \param option
/// \param painter
/// \param widget
///
void AppStyle::drawControl(ControlElement element, const QStyleOption *option,
                           QPainter *painter, const QWidget *widget) const
{
    if (element == CE_ToolButtonLabel) {
        if (const auto *toolButton = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            const QColor windowColor = toolButton->palette.color(QPalette::Window);
            if (windowColor.lightness() < 128) {
                QStyleOptionToolButton adjustedOption(*toolButton);
                const QColor textColor = adjustedOption.palette.color(QPalette::ButtonText);
                adjustedOption.palette.setColor(QPalette::ButtonText, textColor);
                adjustedOption.palette.setBrush(QPalette::Current, QPalette::ButtonText, textColor);
                QProxyStyle::drawControl(element, &adjustedOption, painter, widget);
                return;
            }
        }
    } else if (element == CE_PushButtonLabel) {
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const QColor windowColor = button->palette.color(QPalette::Window);
            if (windowColor.lightness() < 128) {
                QStyleOptionButton adjustedOption(*button);
                const QColor textColor = adjustedOption.palette.color(QPalette::ButtonText);
                adjustedOption.palette.setColor(QPalette::ButtonText, textColor);
                adjustedOption.palette.setBrush(QPalette::Current, QPalette::ButtonText, textColor);
                QProxyStyle::drawControl(element, &adjustedOption, painter, widget);
                return;
            }
        }
    }

    QProxyStyle::drawControl(element, option, painter, widget);
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
