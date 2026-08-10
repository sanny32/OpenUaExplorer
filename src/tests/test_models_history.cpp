// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models_history.cpp
/// \brief Tests HistoryModel behavior.
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
class TestModelsHistory : public QObject
{
    Q_OBJECT

private slots:
    void historyModelHeaderRolesAndMutators();
    void historyModelExportsCsv();
};

///
/// \brief HistoryModel: headerData, value/status columns, alignment and mutators.
///
void TestModelsHistory::historyModelHeaderRolesAndMutators()
{
    HistoryModel model;
    model.setItems(TestData::historyItems());

    QCOMPARE(model.headerData(HistoryModel::ColSourceTimestamp, Qt::Horizontal).toString(),
             QStringLiteral("Source Timestamp"));
    QCOMPARE(model.headerData(HistoryModel::ColValue, Qt::Horizontal).toString(),
             QStringLiteral("Value"));
    QCOMPARE(model.headerData(HistoryModel::ColStatus, Qt::Horizontal).toString(),
             QStringLiteral("Status"));
    QVERIFY(!model.headerData(99, Qt::Horizontal).isValid());
    QVERIFY(!model.headerData(HistoryModel::ColValue, Qt::Horizontal,
                              Qt::DecorationRole).isValid());

    QCOMPARE(model.data(model.index(0, HistoryModel::ColNumber)).toString(),
             QStringLiteral("1"));
    QCOMPARE(model.data(model.index(0, HistoryModel::ColStatus)).toString(),
             QStringLiteral("Good"));
    QVERIFY(model.data(model.index(0, HistoryModel::ColValue),
                       Qt::TextAlignmentRole).isValid());
    QVERIFY(!model.data(QModelIndex()).isValid());

    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
    model.setColumnAlignment(HistoryModel::ColValue,
                             Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(model.data(model.index(0, HistoryModel::ColValue),
                        Qt::TextAlignmentRole).toInt(),
             int(Qt::AlignRight | Qt::AlignVCenter));

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

///
/// \brief HistoryModel exports displayed values as escaped CSV.
///
void TestModelsHistory::historyModelExportsCsv()
{
    OpcUaHistoryValue value;
    value.sourceTimestamp = QDateTime(QDate(2024, 1, 2), QTime(3, 4, 5, 6), QTimeZone::UTC);
    value.serverTimestamp = QDateTime(QDate(2024, 1, 2), QTime(3, 4, 6, 7), QTimeZone::UTC);
    value.value = QStringLiteral("12,\"quoted\"\nline");
    value.status = QStringLiteral("Good,Clamped");

    HistoryModel model;
    model.setTimestampMode(AppSettings::TimestampMode::Utc);
    model.setItems({value});

    QCOMPARE(model.toCsv(),
             QStringLiteral("#,Source Timestamp,Server Timestamp,Value,Status\n"
                            "1,2024-01-02 03:04:05.006Z,2024-01-02 03:04:06.007Z,"
                            "\"12,\"\"quoted\"\"\nline\",\"Good,Clamped\"\n"));
}


QTEST_MAIN(TestModelsHistory)

#include "test_models_history.moc"
