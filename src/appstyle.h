// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appstyle.h
/// \brief Declares the application proxy style.
///

#pragma once

#include <QProxyStyle>

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

    static constexpr int textHMargin = 6;
    static constexpr int textVMargin = 1;
};
