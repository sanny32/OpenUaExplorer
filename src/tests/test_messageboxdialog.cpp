// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_messageboxdialog.cpp
/// \brief Tests the themed message dialog.
///

#include <QLabel>
#include <QTest>

#include "application.h"
#include "dialogs/messageboxdialog.h"

class TestMessageBoxDialog : public QObject
{
    Q_OBJECT

private slots:
    void informativeTextAppearsOnlyWhenSet();
    void longInformativeTextIsElidedIntoOneLine();

private:
    static QLabel *informativeLabel(const MessageBoxDialog &dialog);
};

///
/// \brief Returns the label carrying the dialog's secondary text.
/// \param dialog Dialog to inspect.
/// \return The informative label, or nullptr when it is missing.
///
QLabel *TestMessageBoxDialog::informativeLabel(const MessageBoxDialog &dialog)
{
    return dialog.findChild<QLabel *>(QStringLiteral("informativeLabel"));
}

///
/// \brief The secondary line stays out of the way of dialogs that do not use it.
///
void TestMessageBoxDialog::informativeTextAppearsOnlyWhenSet()
{
    MessageBoxDialog dialog;
    dialog.setText(QStringLiteral("The session has been saved."));
    QLabel *label = informativeLabel(dialog);
    QVERIFY(label);
    QVERIFY(label->isHidden());

    dialog.setInformativeText(QStringLiteral("/tmp/session.ouas"));
    QVERIFY(!label->isHidden());
    QCOMPARE(label->text(), QStringLiteral("/tmp/session.ouas"));
    QCOMPARE(label->toolTip(), QStringLiteral("/tmp/session.ouas"));

    dialog.setInformativeText(QString());
    QVERIFY(label->isHidden());
}

///
/// \brief A path too long for the dialog is shortened instead of wrapped.
///
void TestMessageBoxDialog::longInformativeTextIsElidedIntoOneLine()
{
    const QString path = QStringLiteral("/Users/tester/Documents/")
        + QStringLiteral("very-long-directory-segment/").repeated(10)
        + QStringLiteral("session.ouas");

    MessageBoxDialog dialog;
    dialog.setInformativeText(path);
    QLabel *label = informativeLabel(dialog);
    QVERIFY(label);

    QVERIFY(!label->wordWrap());
    QVERIFY(label->text().length() < path.length());
    QVERIFY(label->text().endsWith(QStringLiteral("session.ouas")));
    QCOMPARE(label->toolTip(), path);
}

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestMessageBoxDialog test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_messageboxdialog.moc"
