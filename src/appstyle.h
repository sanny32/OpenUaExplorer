#pragma once

#include <QProxyStyle>

class AppStyle : public QProxyStyle
{
    Q_OBJECT

public:
    explicit AppStyle(QStyle *style = nullptr);

    QRect subElementRect(SubElement element, const QStyleOption *option,
                         const QWidget *widget = nullptr) const override;

    static constexpr int textHMargin = 6;
    static constexpr int textVMargin = 1;
};
