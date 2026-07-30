// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dialogbuttonbox.cpp
/// \brief Tests the standard dialog button sizing policy.
///

#include <QDialogButtonBox>
#include <QFont>
#include <QPushButton>
#include <QTest>

#include "application.h"
#include "widgets/dialogbuttonbox.h"

///
/// \brief Verifies DialogButtonBox sizing.
///
class TestDialogButtonBox : public QObject
{
    Q_OBJECT

private slots:
    void okAndCancelUseTheSameMinimumWidth();
    void applyKeepsItsNaturalWidth();
    void buttonWidthsFollowFontChanges();
};

///
/// \brief Ok and Cancel both use the wider label's natural button width.
///
void TestDialogButtonBox::okAndCancelUseTheSameMinimumWidth()
{
    DialogButtonBox buttons;
    QFont font = buttons.font();
    font.setPixelSize(48);
    buttons.setFont(font);
    buttons.setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QPushButton *ok = buttons.button(QDialogButtonBox::Ok);
    QPushButton *cancel = buttons.button(QDialogButtonBox::Cancel);
    QVERIFY(ok);
    QVERIFY(cancel);
    QVERIFY(cancel->fontMetrics().horizontalAdvance(cancel->text())
            > ok->fontMetrics().horizontalAdvance(ok->text()));
    QCOMPARE(ok->minimumWidth(), cancel->minimumWidth());
    QVERIFY(ok->minimumWidth() >= cancel->sizeHint().width());
}

///
/// \brief Apply remains content-sized instead of joining the Ok/Cancel pair.
///
void TestDialogButtonBox::applyKeepsItsNaturalWidth()
{
    DialogButtonBox buttons;
    buttons.setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel
                               | QDialogButtonBox::Apply);

    QPushButton *apply = buttons.button(QDialogButtonBox::Apply);
    QVERIFY(apply);
    QCOMPARE(apply->minimumWidth(), 0);
}

///
/// \brief Changing the inherited font recalculates the paired minimum width.
///
void TestDialogButtonBox::buttonWidthsFollowFontChanges()
{
    DialogButtonBox buttons;
    buttons.setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *ok = buttons.button(QDialogButtonBox::Ok);
    QPushButton *cancel = buttons.button(QDialogButtonBox::Cancel);
    QVERIFY(ok);
    QVERIFY(cancel);
    const int initialWidth = ok->minimumWidth();

    QFont font = buttons.font();
    font.setPixelSize(48);
    buttons.setFont(font);

    QVERIFY(ok->minimumWidth() > initialWidth);
    QCOMPARE(ok->minimumWidth(), cancel->minimumWidth());
}

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestDialogButtonBox test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_dialogbuttonbox.moc"
