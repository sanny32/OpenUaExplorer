// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file csvexporter.h
/// \brief Declares CSV export helpers for Qt item models.
///

#pragma once

#include <QString>

class QAbstractItemModel;

namespace CsvExporter {
///
/// \brief Exports the display text of a table model with a horizontal header.
/// \param model Model to export.
/// \return CSV document with a header row.
///
QString tableToCsv(const QAbstractItemModel &model);
}
