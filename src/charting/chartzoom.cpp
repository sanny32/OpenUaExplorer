// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file chartzoom.cpp
/// \brief Implements the mouse wheel zoom arithmetic.
///

#include "chartzoom.h"

#include <cmath>

namespace {

constexpr qreal kNotchDegrees = 120.0;
constexpr qreal kNotchFactor = 1.25;

} // namespace

namespace ChartZoom {

///
/// \brief Converts a mouse wheel rotation into a range scale factor.
/// \param angleDelta Wheel rotation in eighths of a degree.
/// \return Multiplier for the visible span; below 1 zooms in, above 1 zooms out.
///
qreal factorFromWheel(int angleDelta)
{
    if (angleDelta == 0)
        return 1.0;
    // powl, not pow: glibc 2.29 revisioned the double entry point, so pow() would
    // raise the glibc the Linux packages need above what the oldest target has.
    const long double exponent = -angleDelta / kNotchDegrees;
    return static_cast<qreal>(std::pow(static_cast<long double>(kNotchFactor), exponent));
}

///
/// \brief Scales a range around an anchor value, clamping the resulting span.
/// \param range Range scaled in place.
/// \param anchor Value kept at its relative position within the range.
/// \param factor Span multiplier.
/// \param minSpan Smallest allowed span.
/// \param maxSpan Largest allowed span, or 0 for no upper limit.
/// \return True when the range changed.
///
bool scaleRange(ChartRange *range, qreal anchor, qreal factor, qreal minSpan, qreal maxSpan)
{
    if (!range || range->span() <= 0.0 || factor <= 0.0)
        return false;

    const qreal span = range->span();
    qreal scaled = span * factor;
    if (maxSpan > 0.0)
        scaled = qMin(scaled, maxSpan);
    scaled = qMax(scaled, minSpan);
    if (qFuzzyCompare(scaled, span))
        return false;

    const qreal ratio = qBound(0.0, (anchor - range->min) / span, 1.0);
    range->min = anchor - ratio * scaled;
    range->max = range->min + scaled;
    return true;
}

} // namespace ChartZoom
