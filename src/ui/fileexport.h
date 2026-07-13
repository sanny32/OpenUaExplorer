// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file fileexport.h
/// \brief Declares the shared "prompt for a file and write it" export helpers.
///

#pragma once

#include <QString>

class QAbstractItemModel;
class QWidget;

///
/// \brief Prompts for a destination file and writes to it, reporting failures to the user.
///
namespace FileExport {

///
/// \brief Returns the file dialog filter offering CSV files.
/// \return Name filter for CSV exports.
///
QString csvFilter();

///
/// \brief Writes UTF-8 text to a file atomically, without any user interaction.
/// \param fileName Destination file.
/// \param text Text written to the file.
/// \param error Set to a human-readable description when the write fails.
/// \return True when the file was written.
///
bool writeTextFile(const QString &fileName, const QString &text, QString *error = nullptr);

///
/// \brief Prompts for a file name and writes UTF-8 text to it atomically.
/// \param parent Parent widget for the file and message dialogs.
/// \param title Caption of the file dialog, reused as the title of any error dialog.
/// \param suggestedName File name proposed in the file dialog.
/// \param filter Name filter offered in the file dialog.
/// \param text Text written to the chosen file.
/// \return True when the file was written; false when the user cancelled or the write failed.
///
bool saveText(QWidget *parent, const QString &title, const QString &suggestedName,
              const QString &filter, const QString &text);

///
/// \brief Prompts for a file name and writes a table model to it as CSV.
/// \param parent Parent widget for the file and message dialogs.
/// \param title Caption of the file dialog, reused as the title of any error dialog.
/// \param suggestedName File name proposed in the file dialog.
/// \param model Model whose displayed rows are exported.
/// \return True when the file was written; false when the user cancelled or the write failed.
///
bool exportModelToCsv(QWidget *parent, const QString &title, const QString &suggestedName,
                      const QAbstractItemModel &model);

} // namespace FileExport
