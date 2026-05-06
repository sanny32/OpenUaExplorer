// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appstyle.h
/// \brief Declares the application proxy style.
///

#pragma once

#include <QProxyStyle>
#include <QSize>

///
/// \brief Application proxy style that adjusts item and header text margins.
///
class AppStyle : public QProxyStyle
{
    Q_OBJECT

public:
    explicit AppStyle(QStyle *style = nullptr);

    QRect subElementRect(SubElement element, const QStyleOption *option,
                         const QWidget *widget = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &contentsSize,
                           const QWidget *widget = nullptr) const override;

    static constexpr int controlMinHeight = 30;
    static constexpr int textHMargin = 6;
    static constexpr int textVMargin = 1;
};
