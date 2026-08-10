// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models_csv_exporter.cpp
/// \brief Tests generic CSV table export.
///

#include <QAbstractItemModelTester>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QMimeData>
#include <QPalette>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTest>
#include <QTimeZone>

#include "appsettings.h"
#include "testdata.h"
#include "testimages.h"
#include "formatters/attributeformatter.h"
#include "opcua/opcuatypes.h"
#include "models/attributesmodel.h"
#include "models/csvexporter.h"
#include "models/dataaccessmodel.h"
#include "models/eventsmodel.h"
#include "models/historymodel.h"
#include "models/logmodel.h"
#include "models/nodeinfomodel.h"
#include "models/referencesmodel.h"
#include "models/subscriptionsmodel.h"
#include "models/valueroles.h"

using TestImages::encodedPng;

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestModelsCsvExporter : public QObject
{
    Q_OBJECT

private slots:
    void csvExporterEscapesDisplayedTable();
    void csvExporterNeutralizesFormulaPrefixes();
    void csvExporterNeutralizesFormulaHeaders();
};

///
/// \brief CsvExporter writes headers and escapes display text consistently.
///
void TestModelsCsvExporter::csvExporterEscapesDisplayedTable()
{
    QStandardItemModel empty(0, 2);
    empty.setHeaderData(0, Qt::Horizontal, QStringLiteral("Plain"));
    empty.setHeaderData(1, Qt::Horizontal, QStringLiteral("Needs,Quote"));
    QCOMPARE(CsvExporter::tableToCsv(empty), QStringLiteral("Plain,\"Needs,Quote\"\n"));

    QStandardItemModel model(1, 4);
    model.setHeaderData(0, Qt::Horizontal, QStringLiteral("Text"));
    model.setHeaderData(1, Qt::Horizontal, QStringLiteral("Comma"));
    model.setHeaderData(2, Qt::Horizontal, QStringLiteral("Quote"));
    model.setHeaderData(3, Qt::Horizontal, QStringLiteral("Break"));
    model.setData(model.index(0, 0), QStringLiteral("plain"));
    model.setData(model.index(0, 1), QStringLiteral("a,b"));
    model.setData(model.index(0, 2), QStringLiteral("a\"b"));
    model.setData(model.index(0, 3), QStringLiteral("a\r\nb"));

    QCOMPARE(CsvExporter::tableToCsv(model),
             QStringLiteral("Text,Comma,Quote,Break\n"
                            "plain,\"a,b\",\"a\"\"b\",\"a\r\nb\"\n"));
}

///
/// \brief CsvExporter defuses fields a spreadsheet would evaluate as a formula.
///
void TestModelsCsvExporter::csvExporterNeutralizesFormulaPrefixes()
{
    QStandardItemModel model(1, 7);
    model.setHeaderData(0, Qt::Horizontal, QStringLiteral("Equals"));
    model.setHeaderData(1, Qt::Horizontal, QStringLiteral("Plus"));
    model.setHeaderData(2, Qt::Horizontal, QStringLiteral("Minus"));
    model.setHeaderData(3, Qt::Horizontal, QStringLiteral("At"));
    model.setHeaderData(4, Qt::Horizontal, QStringLiteral("Tab"));
    model.setHeaderData(5, Qt::Horizontal, QStringLiteral("Inner"));
    model.setHeaderData(6, Qt::Horizontal, QStringLiteral("Plain"));
    model.setData(model.index(0, 0), QStringLiteral("=1+1"));
    model.setData(model.index(0, 1), QStringLiteral("+A1"));
    // A negative reading is quoted too: telling it from a formula would need a number parser,
    // and a leading apostrophe costs a spreadsheet user nothing.
    model.setData(model.index(0, 2), QStringLiteral("-2"));
    model.setData(model.index(0, 3), QStringLiteral("@SUM(A1)"));
    model.setData(model.index(0, 4), QStringLiteral("\tx"));
    model.setData(model.index(0, 5), QStringLiteral("a=b"));
    model.setData(model.index(0, 6), QStringLiteral("plain"));

    QCOMPARE(CsvExporter::tableToCsv(model),
             QStringLiteral("Equals,Plus,Minus,At,Tab,Inner,Plain\n"
                            "'=1+1,'+A1,'-2,'@SUM(A1),'\tx,a=b,plain\n"));
}

///
/// \brief A formula-shaped header is defused just like a cell.
///
void TestModelsCsvExporter::csvExporterNeutralizesFormulaHeaders()
{
    QStandardItemModel model(0, 2);
    model.setHeaderData(0, Qt::Horizontal, QStringLiteral("=cmd"));
    model.setHeaderData(1, Qt::Horizontal, QStringLiteral("Plain"));

    QCOMPARE(CsvExporter::tableToCsv(model), QStringLiteral("'=cmd,Plain\n"));
}


QTEST_MAIN(TestModelsCsvExporter)

#include "test_models_csv_exporter.moc"
