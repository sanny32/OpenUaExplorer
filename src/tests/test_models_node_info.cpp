// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models_node_info.cpp
/// \brief Tests NodeInfoModel behavior.
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
class TestModelsNodeInfo : public QObject
{
    Q_OBJECT

private slots:
    void nodeInfoModelColumnsAndClear();
};

///
/// \brief NodeInfoModel: the value column, a non-display role and clear().
///
void TestModelsNodeInfo::nodeInfoModelColumnsAndClear()
{
    NodeInfoModel model;
    model.setItems(TestData::nodeInfoItems());
    QVERIFY(model.rowCount() > 0);

    QCOMPARE(model.data(model.index(0, NodeInfoModel::ColValue)).toString(),
             TestData::nodeInfoItems().first().value);
    QVERIFY(!model.data(model.index(0, NodeInfoModel::ColLabel),
                        Qt::ToolTipRole).isValid());

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}


QTEST_MAIN(TestModelsNodeInfo)

#include "test_models_node_info.moc"
