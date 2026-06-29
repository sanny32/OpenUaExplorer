// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file chartviewfactory.cpp
/// \brief Selects and constructs the active chart backend.
///

#include "chartviewfactory.h"

#include "backends/qtcharts/qtchartsview.h"

namespace ChartViewFactory {

std::unique_ptr<IChartView> createChartView(QWidget *parent)
{
    return std::make_unique<QtChartsView>(parent);
}

} // namespace ChartViewFactory
