// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file chartviewfactory.h
/// \brief Declares the factory that creates the active chart backend.
///

#pragma once

#include <memory>

class IChartView;
class QWidget;

///
/// \brief Single creation point for chart views.
///
/// This is the only place that selects a charting backend. Swapping Qt Charts
/// for another renderer means adding a sibling under backends/ and changing the
/// implementation here; no consumer code changes.
///
namespace ChartViewFactory {

///
/// \brief Creates a chart view backed by the active rendering backend.
/// \param parent Parent for the underlying widget; may be nullptr.
/// \return Owning pointer to a new chart view.
///
std::unique_ptr<IChartView> createChartView(QWidget *parent = nullptr);

} // namespace ChartViewFactory
