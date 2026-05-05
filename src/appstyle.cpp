#include <QPainter>
#include <QPalette>
#include <QImage>
#include <QPixmap>
#include <QStyleOptionButton>
#include <QStyleOptionToolButton>
#include <QWidget>

#include "appstyle.h"

namespace {

///
/// \brief tintedIcon
/// \param icon
/// \param size
/// \param color
/// \return
///
QIcon tintedIcon(const QIcon &icon, const QSize &size, const QColor &color)
{
    QPixmap pixmap = icon.pixmap(size);
    QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);

    for (int y = 0; y < image.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const int alpha = qAlpha(line[x]);
            if (alpha > 0) {
                line[x] = qRgba(color.red(), color.green(), color.blue(), alpha);
            }
        }
    }

    return QIcon(QPixmap::fromImage(image));
}

}

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
                QStyleOptionToolButton adjustedOption(*toolButton);
                const QColor textColor = adjustedOption.palette.color(QPalette::ButtonText);
                adjustedOption.palette.setColor(QPalette::ButtonText, textColor);
                adjustedOption.palette.setBrush(QPalette::Current, QPalette::ButtonText, textColor);
                if (!adjustedOption.icon.isNull()) {
                    adjustedOption.icon = tintedIcon(adjustedOption.icon, adjustedOption.iconSize, textColor);
                }
                QProxyStyle::drawControl(element, &adjustedOption, painter, widget);
                return;
        }
    } else if (element == CE_PushButtonLabel) {
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const QColor windowColor = button->palette.color(QPalette::Window);
                QStyleOptionButton adjustedOption(*button);
                const QColor textColor = adjustedOption.palette.color(QPalette::ButtonText);
                adjustedOption.palette.setColor(QPalette::ButtonText, textColor);
                adjustedOption.palette.setBrush(QPalette::Current, QPalette::ButtonText, textColor);
                QProxyStyle::drawControl(element, &adjustedOption, painter, widget);
                return;
        }
    }

    QProxyStyle::drawControl(element, option, painter, widget);
}

///
/// \brief AppStyle::drawComplexControl
/// \param control
/// \param option
/// \param painter
/// \param widget
///
void AppStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                  QPainter *painter, const QWidget *widget) const
{
    if (control == CC_ToolButton && widget && widget->inherits("QDockWidgetTitleButton") && option) {
        const bool closeButton = widget->objectName() == "qt_dockwidget_closebutton";
        const bool floatButton = widget->objectName() == "qt_dockwidget_floatbutton";
        if (closeButton || floatButton) {
            if (const auto *toolButton = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
                QStyleOptionToolButton adjustedOption(*toolButton);
                adjustedOption.icon = tintedIcon(toolButton->icon, toolButton->iconSize,
                                                 option->palette.color(QPalette::ButtonText));
                QProxyStyle::drawComplexControl(control, &adjustedOption, painter, widget);
                return;
            }
        }
    }

    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

///
/// \brief AppStyle::drawPrimitive
/// \param element
/// \param option
/// \param painter
/// \param widget
///
void AppStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                             QPainter *painter, const QWidget *widget) const
{
    if (element == PE_PanelItemViewItem && option
        && (option->state & State_Selected)) {
        painter->fillRect(option->rect, option->palette.color(QPalette::Highlight));
        return;
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
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

///
/// \brief AppStyle::styleHint
/// \param hint
/// \param option
/// \param widget
/// \param returnData
/// \return
///
int AppStyle::styleHint(StyleHint hint, const QStyleOption *option,
                        const QWidget *widget, QStyleHintReturn *returnData) const
{
    if (hint == SH_Table_GridLineColor && option) {
        return int(option->palette.color(QPalette::Mid).rgba());
    }

    return QProxyStyle::styleHint(hint, option, widget, returnData);
}

///
/// \brief AppStyle::standardIcon
/// \param standardIcon
/// \param option
/// \param widget
/// \return
///
QIcon AppStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *option,
                             const QWidget *widget) const
{
    if (option) {
        const bool closeIcon = standardIcon == SP_TitleBarCloseButton
            || standardIcon == SP_DockWidgetCloseButton;
        const bool floatIcon = standardIcon == SP_TitleBarNormalButton;
        if (closeIcon || floatIcon) {
            const QIcon icon = QProxyStyle::standardIcon(standardIcon, option, widget);
            return tintedIcon(icon, QSize(16, 16), option->palette.color(QPalette::ButtonText));
        }
    }

    return QProxyStyle::standardIcon(standardIcon, option, widget);
}
