// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_writevaluedialog.cpp
/// \brief UI tests for the OPC UA write-value dialog.
///

#include <QLabel>
#include <QTest>

#include <QtOpcUa/qopcuatype.h>

#include "dialogs/writevaluedialog.h"

///
/// \brief Tests WriteValueDialog value seeding and display formatting.
///
class TestWriteValueDialog : public QObject
{
    Q_OBJECT

private slots:
    void dataTypeLabelUsesDisplayName();
};

void TestWriteValueDialog::dataTypeLabelUsesDisplayName()
{
    WriteValueDialog dialog;
    auto *dataTypeLabel = dialog.findChild<QLabel *>(QStringLiteral("dataTypeLabel"));
    QVERIFY(dataTypeLabel);

    dialog.setValue(1.25, int(QOpcUa::Types::Double), QStringLiteral("ns=0;i=11"), true);
    QCOMPARE(dataTypeLabel->text(), QStringLiteral("Double"));

    dialog.setValue(QStringLiteral("custom"), int(QOpcUa::Types::String),
                    QStringLiteral("ns=3;i=5001"), true);
    QCOMPARE(dataTypeLabel->text(), QStringLiteral("ns=3;i=5001"));
}

QTEST_MAIN(TestWriteValueDialog)

#include "test_writevaluedialog.moc"
