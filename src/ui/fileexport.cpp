// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file fileexport.cpp
/// \brief Implements the shared "prompt for a file and write it" export helpers.
///

#include <QCoreApplication>
#include <QFileDialog>
#include <QSaveFile>
#include <QTextStream>

#include "dialogs/messageboxdialog.h"
#include "fileexport.h"
#include "models/csvexporter.h"

///
/// \brief Returns the file dialog filter offering CSV files.
/// \return Name filter for CSV exports.
///
QString FileExport::csvFilter()
{
    return QCoreApplication::translate("FileExport", "CSV Files (*.csv);;All Files (*)");
}

///
/// \brief Writes UTF-8 text to a file atomically, without any user interaction.
/// \param fileName Destination file.
/// \param text Text written to the file.
/// \param error Set to a human-readable description when the write fails.
/// \return True when the file was written.
///
bool FileExport::writeTextFile(const QString &fileName, const QString &text, QString *error)
{
    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) {
            *error = QCoreApplication::translate("FileExport",
                                                 "Could not open '%1' for writing.").arg(fileName);
        }
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << text;
    if (!file.commit()) {
        if (error)
            *error = QCoreApplication::translate("FileExport", "Could not save '%1'.").arg(fileName);
        return false;
    }
    return true;
}

///
/// \brief Prompts for a file name and writes UTF-8 text to it atomically.
/// \param parent Parent widget for the file and message dialogs.
/// \param title Caption of the file dialog, reused as the title of any error dialog.
/// \param suggestedName File name proposed in the file dialog.
/// \param filter Name filter offered in the file dialog.
/// \param text Text written to the chosen file.
/// \return True when the file was written; false when the user cancelled or the write failed.
///
bool FileExport::saveText(QWidget *parent, const QString &title, const QString &suggestedName,
                          const QString &filter, const QString &text)
{
    const QString fileName = QFileDialog::getSaveFileName(parent, title, suggestedName, filter);
    if (fileName.isEmpty())
        return false;

    QString error;
    if (!writeTextFile(fileName, text, &error)) {
        MessageBoxDialog::warning(parent, title, error, DialogButtonBox::Ok);
        return false;
    }
    return true;
}

///
/// \brief Prompts for a file name and writes a table model to it as CSV.
/// \param parent Parent widget for the file and message dialogs.
/// \param title Caption of the file dialog, reused as the title of any error dialog.
/// \param suggestedName File name proposed in the file dialog.
/// \param model Model whose displayed rows are exported.
/// \return True when the file was written; false when the user cancelled or the write failed.
///
bool FileExport::exportModelToCsv(QWidget *parent, const QString &title,
                                  const QString &suggestedName, const QAbstractItemModel &model)
{
    return saveText(parent, title, suggestedName, csvFilter(), CsvExporter::tableToCsv(model));
}
