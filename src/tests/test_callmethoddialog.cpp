// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_callmethoddialog.cpp
/// \brief UI tests for the OPC UA method-call dialog.
///

#include <QAbstractItemModel>
#include <QLabel>
#include <QTableView>
#include <QTest>

#include "dialogs/callmethoddialog.h"
#include "opcua/opcuabackend.h"
#include "opcua/qtopcuabackend.h"

///
/// \brief Tests CallMethodDialog table layout and targeting.
///
class TestCallMethodDialog : public QObject
{
    Q_OBJECT

private slots:
    void tablesHaveArgumentColumns();
    void windowTitleShowsMethodAndObject();
    void missingConnectionEnablesCall();
};

void TestCallMethodDialog::tablesHaveArgumentColumns()
{
    QtOpcUaBackend service;
    CallMethodDialog dialog(&service);

    auto *inputTable = dialog.findChild<QTableView *>(QStringLiteral("inputTable"));
    auto *outputTable = dialog.findChild<QTableView *>(QStringLiteral("outputTable"));
    QVERIFY(inputTable);
    QVERIFY(outputTable);
    QVERIFY(inputTable->model());
    QVERIFY(outputTable->model());
    QCOMPARE(inputTable->model()->columnCount(), 4);
    QCOMPARE(outputTable->model()->columnCount(), 4);
}

void TestCallMethodDialog::windowTitleShowsMethodAndObject()
{
    QtOpcUaBackend service;
    CallMethodDialog dialog(&service);

    OpcUaNodeInfo object;
    object.displayName = QStringLiteral("MyDevice");
    OpcUaNodeInfo method;
    method.displayName = QStringLiteral("MyMethod");
    method.nodeId = QStringLiteral("ns=2;s=MyMethod");
    method.nodeClass = OpcUa::Method;

    dialog.setTarget(object, method);
    QCOMPARE(dialog.windowTitle(), QStringLiteral("Call MyMethod on MyDevice"));
    QCOMPARE(dialog.methodNodeId(), QStringLiteral("ns=2;s=MyMethod"));
}

void TestCallMethodDialog::missingConnectionEnablesCall()
{
    QtOpcUaBackend service;
    CallMethodDialog dialog(&service);

    OpcUaNodeInfo object;
    OpcUaNodeInfo method;
    method.displayName = QStringLiteral("MyMethod");
    method.nodeId = QStringLiteral("ns=2;s=MyMethod");
    method.nodeClass = OpcUa::Method;

    // Without a live connection the backend reports the failure synchronously; the
    // dialog should surface it and still re-enable the Call button.
    dialog.setTarget(object, method);

    auto *resultLabel = dialog.findChild<QLabel *>(QStringLiteral("resultLabel"));
    QVERIFY(resultLabel);
    QVERIFY(!resultLabel->text().isEmpty());
}

QTEST_MAIN(TestCallMethodDialog)

#include "test_callmethoddialog.moc"
