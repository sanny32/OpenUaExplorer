#include "coloredpushbutton.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOption>

ColoredPushButton::ColoredPushButton(QWidget *parent)
    : QPushButton(parent)
{
}

void ColoredPushButton::setColors(const Colors &colors)
{
    m_colors = colors;
    m_colored = true;

    QPalette pal = palette();
    pal.setColor(QPalette::Button, colors.base);
    pal.setColor(QPalette::ButtonText, colors.text);
    setPalette(pal);

    update();
}

void ColoredPushButton::clearColors()
{
    m_colored = false;
    setPalette(QApplication::palette());
    update();
}

void ColoredPushButton::paintEvent(QPaintEvent *event)
{
    if (!m_colored) {
        QPushButton::paintEvent(event);
        return;
    }

    QStyleOption opt;
    opt.initFrom(this);

    const bool down = isDown() || isChecked();
    const bool hovered = opt.state & QStyle::State_MouseOver;

    QColor bg = m_colors.base;
    if (down)
        bg = m_colors.pressed;
    else if (hovered)
        bg = m_colors.hover;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1), 4, 4);

    p.fillPath(path, bg);

    if (hasFocus()) {
        p.setPen(QPen(bg.lighter(160), 1.5));
        p.drawPath(path);
    }

    p.setPen(m_colors.text);
    p.drawText(rect(), Qt::AlignCenter, text());
}
