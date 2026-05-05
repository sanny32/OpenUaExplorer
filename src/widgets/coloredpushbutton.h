#pragma once

#include <QColor>
#include <QPushButton>

class ColoredPushButton : public QPushButton
{
    Q_OBJECT

public:
    struct Colors {
        QColor base;
        QColor hover;
        QColor pressed;
        QColor text = Qt::white;
    };

    explicit ColoredPushButton(QWidget *parent = nullptr);

    void setColors(const Colors &colors);
    void clearColors();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Colors m_colors;
    bool m_colored = false;
};
