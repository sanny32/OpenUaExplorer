// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models_attributes.cpp
/// \brief Tests AttributesModel behavior.
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
class TestModelsAttributes : public QObject
{
    Q_OBJECT

private slots:
    void attributesModelOffersImageBytes();
    void attributesModelTimestampModeReformats();
    void attributesModelKeepsEmptyValueEmpty();
    void attributesModelOfflineGreysValues();
    void attributesModelHeaderRolesAndMutators();
};

///
/// \brief An image value row in the attributes tree hands its bytes to the viewer.
///
void TestModelsAttributes::attributesModelOffersImageBytes()
{
    const QByteArray png = encodedPng(QSize(3, 2));
    QVERIFY(!png.isEmpty());

    AttributesModel model;
    OpcUaNodeAttribute parent;
    parent.name = QStringLiteral("Value");
    parent.displayValue = QStringLiteral("ImagePNG");
    parent.children.append(OpcUaFormat::valueAttribute(QVariant(png), QOpcUa::Types::ByteString,
                                                       QStringLiteral("ns=0;i=2003")));
    model.setAttributes({parent});

    const QModelIndex row = model.index(0, 0);
    QCOMPARE(model.rowCount(row), 1);
    const QModelIndex value = model.index(0, AttributesModel::ColValue, row);
    QVERIFY2(model.data(value).toString().startsWith(QStringLiteral("PNG 3×2, ")),
             qPrintable(model.data(value).toString()));
    QCOMPARE(model.data(value, ValueRoles::ImageDataRole).toByteArray(), png);

    // The attribute-name column is not where the picture lives.
    QVERIFY(model.data(model.index(0, AttributesModel::ColAttribute, row),
                       ValueRoles::ImageDataRole).isNull());
}

///
/// \brief Toggling the timestamp mode reformats timestamp rows in the attributes tree.
///
void TestModelsAttributes::attributesModelTimestampModeReformats()
{
    OpcUaNodeAttribute value;
    value.name = QStringLiteral("Value");
    value.displayValue = QStringLiteral("42");
    OpcUaNodeAttribute timestamp;
    timestamp.name = QStringLiteral("Source Timestamp");
    timestamp.sourceTimestamp = QDateTime(QDate(2024, 1, 2), QTime(3, 4, 5, 678), QTimeZone::UTC);
    timestamp.isTimestamp = true;
    value.children.append(timestamp);

    AttributesModel model;
    model.setAttributes({value});

    const QModelIndex valueParent = model.index(0, 0);
    const QModelIndex timestampIndex = model.index(0, AttributesModel::ColValue, valueParent);

    model.setTimestampMode(AppSettings::TimestampMode::LocalTime);
    const QDateTime local = timestamp.sourceTimestamp.toLocalTime();
    QCOMPARE(model.data(timestampIndex).toString(),
             local.toOffsetFromUtc(local.offsetFromUtc()).toString(Qt::ISODateWithMs)
                 .replace(QLatin1Char('T'), QLatin1Char(' ')));

    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
    model.setTimestampMode(AppSettings::TimestampMode::Utc);
    QVERIFY(spy.count() >= 1);
    const QString utc = model.data(timestampIndex).toString();
    QCOMPARE(utc, timestamp.sourceTimestamp.toUTC().toString(Qt::ISODateWithMs)
                      .replace(QLatin1Char('T'), QLatin1Char(' ')));
    QVERIFY(utc.endsWith(QLatin1Char('Z')));
    QVERIFY(!utc.contains(QLatin1Char('T')));
}

///
/// \brief An attribute with no value stays empty instead of borrowing a read timestamp.
///
void TestModelsAttributes::attributesModelKeepsEmptyValueEmpty()
{
    OpcUaNodeAttribute description;
    description.name = QStringLiteral("Description");
    description.sourceTimestamp = QDateTime(QDate(2024, 1, 2), QTime(3, 4, 5, 678), QTimeZone::UTC);
    description.serverTimestamp = description.sourceTimestamp;

    AttributesModel model;
    model.setAttributes({description});

    QCOMPARE(model.data(model.index(0, AttributesModel::ColValue)).toString(), QString());
}

///
/// \brief A lost connection greys the attribute values it was read with (issue #7).
///
void TestModelsAttributes::attributesModelOfflineGreysValues()
{
    OpcUaNodeAttribute value;
    value.name = QStringLiteral("Value");
    value.displayValue = QStringLiteral("Good");

    AttributesModel model;
    model.setAttributes({value});

    const QModelIndex valueIndex = model.index(0, AttributesModel::ColValue);
    QCOMPARE(model.data(valueIndex, Qt::ForegroundRole).value<QBrush>().color(),
             QColor(0, 150, 64));

    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
    model.setOffline(true);
    QVERIFY(spy.count() >= 1);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(valueIndex, Qt::ForegroundRole).value<QBrush>().color(),
             qApp->palette().color(QPalette::Disabled, QPalette::Text));

    model.setOffline(false);
    QCOMPARE(model.data(valueIndex, Qt::ForegroundRole).value<QBrush>().color(),
             QColor(0, 150, 64));
}

///
/// \brief AttributesModel: headerData, the foreground-colour rules and mutators.
///
void TestModelsAttributes::attributesModelHeaderRolesAndMutators()
{
    AttributesModel model;
    new QAbstractItemModelTester(&model, &model);

    QCOMPARE(model.headerData(AttributesModel::ColAttribute, Qt::Horizontal).toString(),
             QStringLiteral("Attribute"));
    QCOMPARE(model.headerData(AttributesModel::ColValue, Qt::Horizontal).toString(),
             QStringLiteral("Value"));
    QVERIFY(!model.headerData(99, Qt::Horizontal).isValid());
    QVERIFY(!model.headerData(AttributesModel::ColAttribute, Qt::Horizontal,
                              Qt::DecorationRole).isValid());

    // Flat rows whose values drive the status-colour rules.
    model.setItems({{QStringLiteral("Status"), QStringLiteral("Good")},
                    {QStringLiteral("Error"),  QStringLiteral("Bad_NodeIdUnknown")},
                    {QStringLiteral("Empty"),  QString()},
                    {QStringLiteral("Plain"),  QStringLiteral("normal")}});
    QCOMPARE(model.rowCount(), 4);

    const QModelIndex goodValue = model.index(0, AttributesModel::ColValue);
    QCOMPARE(model.data(goodValue).toString(), QStringLiteral("Good"));
    QCOMPARE(model.data(goodValue, Qt::ForegroundRole).value<QBrush>().color(),
             QColor(0, 150, 64));
    QCOMPARE(model.data(model.index(1, AttributesModel::ColValue), Qt::ForegroundRole)
                 .value<QBrush>().color(), QColor(210, 70, 70));
    QCOMPARE(model.data(model.index(2, AttributesModel::ColValue), Qt::ForegroundRole)
                 .value<QBrush>().color(), QColor(128, 128, 128));
    // A plain value and the attribute column never get a foreground brush.
    QVERIFY(!model.data(model.index(3, AttributesModel::ColValue),
                        Qt::ForegroundRole).isValid());
    QVERIFY(!model.data(model.index(0, AttributesModel::ColAttribute),
                        Qt::ForegroundRole).isValid());

    QVERIFY(model.data(goodValue, Qt::TextAlignmentRole).isValid());
    QVERIFY(!model.data(QModelIndex()).isValid());
    // A top-level item's parent is the invalid root; parent of an invalid index too.
    QVERIFY(!model.parent(model.index(0, 0)).isValid());
    QVERIFY(!model.parent(QModelIndex()).isValid());
    // Out-of-range requests and rowCount on a value-column parent return empties.
    QVERIFY(!model.index(99, 0).isValid());
    QCOMPARE(model.rowCount(model.index(0, AttributesModel::ColValue)), 0);

    model.setColumnAlignment(AttributesModel::ColValue,
                             Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
    QCOMPARE(model.data(goodValue, Qt::TextAlignmentRole).toInt(),
             int(Qt::AlignRight | Qt::AlignVCenter));

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}


QTEST_MAIN(TestModelsAttributes)

#include "test_models_attributes.moc"
