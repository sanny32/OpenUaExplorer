// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_updatechecker.cpp
/// \brief Unit tests for update version comparison.
///

#include <QTest>

#include "updatechecker.h"

///
/// \brief Tests the semantic version comparison used to detect updates.
///
class TestUpdateChecker : public QObject
{
    Q_OBJECT

private slots:
    void comparesNumericVersions();
    void comparesPreReleaseSuffixes();
    void rejectsInvalidCandidates();
};

///
/// \brief Verifies ordering of plain numeric versions.
///
void TestUpdateChecker::comparesNumericVersions()
{
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("1.2.1"), QStringLiteral("1.2.0")));
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("2.0.0"), QStringLiteral("1.9.9")));
    QVERIFY(!UpdateChecker::isVersionNewer(QStringLiteral("1.2.0"), QStringLiteral("1.2.0")));
    QVERIFY(!UpdateChecker::isVersionNewer(QStringLiteral("1.1.0"), QStringLiteral("1.2.0")));
}

///
/// \brief Verifies pre-release suffixes rank below the matching stable release.
///
void TestUpdateChecker::comparesPreReleaseSuffixes()
{
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("1.2.0"), QStringLiteral("1.2.0-rc1")));
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("1.2.0-rc2"), QStringLiteral("1.2.0-rc1")));
    QVERIFY(UpdateChecker::isVersionNewer(QStringLiteral("1.2.0-beta1"), QStringLiteral("1.2.0-alpha3")));
    QVERIFY(!UpdateChecker::isVersionNewer(QStringLiteral("1.2.0-beta1"), QStringLiteral("1.2.0")));
    QVERIFY(!UpdateChecker::isVersionNewer(QStringLiteral("1.2.0-alpha1"), QStringLiteral("1.2.0-rc1")));
}

///
/// \brief Verifies unparseable candidates are never treated as newer.
///
void TestUpdateChecker::rejectsInvalidCandidates()
{
    QVERIFY(!UpdateChecker::isVersionNewer(QString(), QStringLiteral("1.0.0")));
    QVERIFY(!UpdateChecker::isVersionNewer(QStringLiteral("not-a-version"), QStringLiteral("1.0.0")));
}

QTEST_MAIN(TestUpdateChecker)

#include "test_updatechecker.moc"
