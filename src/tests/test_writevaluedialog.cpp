// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_writevaluedialog.cpp
/// \brief UI tests for the OPC UA write-value dialog.
///

#include <QCheckBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTest>

#include <QtOpcUa/qopcuatype.h>

#include "dialogs/writevaluedialog.h"
#include "widgets/dialogbuttonbox.h"

///
/// \brief Tests WriteValueDialog value seeding and display formatting.
///
class TestWriteValueDialog : public QObject
{
    Q_OBJECT

private slots:
    void dataTypeLabelUsesDisplayName();
    void byteValuesAreShownAsNumbers();
    void byteArraysAreShownAsNumbers();
    void shownByteIsWrittenBackUnchanged();
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

///
/// \brief Byte and SByte scalars are seeded as numbers, not as their character.
///
void TestWriteValueDialog::byteValuesAreShownAsNumbers()
{
    WriteValueDialog dialog;
    auto *valueEdit = dialog.findChild<QPlainTextEdit *>(QStringLiteral("valueEdit"));
    QVERIFY(valueEdit);

    dialog.setValue(QVariant::fromValue<qint8>(97), int(QOpcUa::Types::SByte),
                    QStringLiteral("ns=0;i=2"), true);
    QCOMPARE(valueEdit->toPlainText(), QStringLiteral("97"));

    dialog.setValue(QVariant::fromValue<qint8>(-5), int(QOpcUa::Types::SByte),
                    QStringLiteral("ns=0;i=2"), true);
    QCOMPARE(valueEdit->toPlainText(), QStringLiteral("-5"));

    dialog.setValue(QVariant::fromValue<quint8>(200), int(QOpcUa::Types::Byte),
                    QStringLiteral("ns=0;i=3"), true);
    QCOMPARE(valueEdit->toPlainText(), QStringLiteral("200"));
}

///
/// \brief Byte and SByte array elements are seeded as JSON numbers.
///
void TestWriteValueDialog::byteArraysAreShownAsNumbers()
{
    WriteValueDialog dialog;
    auto *valueEdit = dialog.findChild<QPlainTextEdit *>(QStringLiteral("valueEdit"));
    auto *arrayCheckBox = dialog.findChild<QCheckBox *>(QStringLiteral("arrayCheckBox"));
    QVERIFY(valueEdit);
    QVERIFY(arrayCheckBox);

    const QVariantList values = {QVariant::fromValue<quint8>(1), QVariant::fromValue<quint8>(97)};
    dialog.setValue(values, int(QOpcUa::Types::Byte), QStringLiteral("ns=0;i=3"), true);

    QVERIFY(arrayCheckBox->isChecked());
    QCOMPARE(valueEdit->toPlainText().simplified(), QStringLiteral("[ 1, 97 ]"));
}

///
/// \brief Accepting an untouched SByte keeps the seeded value.
///
void TestWriteValueDialog::shownByteIsWrittenBackUnchanged()
{
    WriteValueDialog dialog;
    auto *buttonBox = dialog.findChild<DialogButtonBox *>(QStringLiteral("buttonBox"));
    QVERIFY(buttonBox);

    dialog.setValue(QVariant::fromValue<qint8>(97), int(QOpcUa::Types::SByte),
                    QStringLiteral("ns=0;i=2"), true);
    QPushButton *okButton = buttonBox->button(DialogButtonBox::Ok);
    QVERIFY(okButton);
    okButton->click();

    QCOMPARE(dialog.result(), int(QDialog::Accepted));
    QCOMPARE(dialog.valueType(), int(QOpcUa::Types::SByte));
    QCOMPARE(dialog.value().toInt(), 97);
}

QTEST_MAIN(TestWriteValueDialog)

#include "test_writevaluedialog.moc"
