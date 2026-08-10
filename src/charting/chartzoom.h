// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file chartzoom.h
/// \brief Declares the backend-neutral mouse wheel zoom arithmetic.
///
/// The zoom policy (how far one wheel notch scales a range, and how a range is
/// scaled around the value under the cursor) is shared by every chart host, so it
/// lives here instead of being repeated in each widget.
///

#pragma once

#include <QtGlobal>

#include "charttypes.h"

namespace ChartZoom {

/// \brief Shortest time window a chart may be zoomed to, in milliseconds.
constexpr qreal kMinTimeSpanMs = 1000.0;
/// \brief Longest time window a chart may be zoomed to, in milliseconds (31 days).
constexpr qreal kMaxTimeSpanMs = 31.0 * 24.0 * 3600.0 * 1000.0;
/// \brief Smallest value span a chart may be zoomed to.
constexpr qreal kMinValueSpan = 1e-9;

///
/// \brief Converts a mouse wheel rotation into a range scale factor.
/// \param angleDelta Wheel rotation in eighths of a degree.
/// \return Multiplier for the visible span; below 1 zooms in, above 1 zooms out.
///
qreal factorFromWheel(int angleDelta);

///
/// \brief Scales a range around an anchor value, clamping the resulting span.
/// \param range Range scaled in place.
/// \param anchor Value kept at its relative position within the range.
/// \param factor Span multiplier.
/// \param minSpan Smallest allowed span.
/// \param maxSpan Largest allowed span, or 0 for no upper limit.
/// \return True when the range changed.
///
bool scaleRange(ChartRange *range, qreal anchor, qreal factor, qreal minSpan, qreal maxSpan);

} // namespace ChartZoom
