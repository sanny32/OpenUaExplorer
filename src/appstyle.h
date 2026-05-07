// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QProxyStyle>
#include <QSize>
#include <QString>

class AppStyle : public QProxyStyle
{
    Q_OBJECT

public:
    explicit AppStyle(const QString &baseStyleName = QString());

    QRect subElementRect(SubElement element, const QStyleOption *option,
                         const QWidget *widget = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &contentsSize,
                           const QWidget *widget = nullptr) const override;

    static constexpr int controlMinHeight = 30;
    static constexpr int textHMargin = 6;
    static constexpr int textVMargin = 1;

    static const QStyle *baseStyle();
    static bool isFusionStyle();
};
