// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models_references.cpp
/// \brief Tests ReferencesModel behavior.
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
class TestModelsReferences : public QObject
{
    Q_OBJECT

private slots:
    void referencesModelHeaderAndEdges();
};

///
/// \brief ReferencesModel: headerData, non-display role and clear().
///
void TestModelsReferences::referencesModelHeaderAndEdges()
{
    ReferencesModel model;
    model.setItems({{QStringLiteral("Organizes"), QStringLiteral("ns=0;i=85")}});

    QCOMPARE(model.headerData(ReferencesModel::ColReference, Qt::Horizontal).toString(),
             QStringLiteral("Reference"));
    QCOMPARE(model.headerData(ReferencesModel::ColTarget, Qt::Horizontal).toString(),
             QStringLiteral("Target"));
    QVERIFY(!model.headerData(99, Qt::Horizontal).isValid());
    QVERIFY(!model.headerData(ReferencesModel::ColReference, Qt::Horizontal,
                              Qt::DecorationRole).isValid());

    QCOMPARE(model.data(model.index(0, ReferencesModel::ColReference)).toString(),
             QStringLiteral("Organizes"));
    // A non-display role yields an empty value.
    QVERIFY(!model.data(model.index(0, ReferencesModel::ColTarget),
                        Qt::ToolTipRole).isValid());
    QVERIFY(!model.data(QModelIndex()).isValid());

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}


QTEST_MAIN(TestModelsReferences)

#include "test_models_references.moc"
