// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_licensebundle.cpp
/// \brief Tests the distributable third-party license bundle.
///

#include <QDir>
#include <QFile>
#include <QTest>

class TestLicenseBundle : public QObject
{
    Q_OBJECT

private slots:
    void containsRequiredFiles();
    void noticeIdentifiesBundledComponents();
};

///
/// \brief Every file installed by the packaging rules is present and non-empty.
///
void TestLicenseBundle::containsRequiredFiles()
{
    const QDir root(QStringLiteral(OUAEXP_SOURCE_DIR));
    const QStringList files = {
        QStringLiteral("LICENSE"),
        QStringLiteral("THIRD_PARTY_NOTICES.md"),
        QStringLiteral("licenses/Apache-2.0.txt"),
        QStringLiteral("licenses/BSD-3-Clause-open62541.txt"),
        QStringLiteral("licenses/BSD-3-Clause-QtKeychain.txt"),
        QStringLiteral("licenses/CC-BY-SA-4.0.txt"),
        QStringLiteral("licenses/CC0-1.0.txt"),
        QStringLiteral("licenses/GPL-3.0-only.txt"),
        QStringLiteral("licenses/LGPL-3.0-only.txt"),
        QStringLiteral("licenses/Lucide-ISC-MIT.txt"),
        QStringLiteral("licenses/MIT-open62541.txt"),
        QStringLiteral("licenses/MIT-Qlementine.txt"),
        QStringLiteral("licenses/MPL-2.0.txt"),
        QStringLiteral("licenses/open62541-AUTHORS.txt"),
        QStringLiteral("licenses/OpenSSL-ACKNOWLEDGEMENTS.md"),
    };

    for (const QString &file : files) {
        QFile input(root.filePath(file));
        QVERIFY2(input.open(QIODevice::ReadOnly), qPrintable(file));
        QVERIFY2(!input.readAll().trimmed().isEmpty(), qPrintable(file));
    }
}

///
/// \brief The notice names every direct dependency shipped in release binaries.
///
void TestLicenseBundle::noticeIdentifiesBundledComponents()
{
    QFile input(QDir(QStringLiteral(OUAEXP_SOURCE_DIR))
                    .filePath(QStringLiteral("THIRD_PARTY_NOTICES.md")));
    QVERIFY(input.open(QIODevice::ReadOnly));
    const QByteArray notice = input.readAll();

    for (const QByteArray &component : {QByteArray("Qt 6"),
                                        QByteArray("Qt Charts"),
                                        QByteArray("QtKeychain"),
                                        QByteArray("OpenSSL"),
                                        QByteArray("open62541"),
                                        QByteArray("Lucide"),
                                        QByteArray("Qlementine")}) {
        QVERIFY2(notice.contains(component), component.constData());
    }
    QVERIFY(!notice.contains("Qlementine Icons"));
}

QTEST_MAIN(TestLicenseBundle)

#include "test_licensebundle.moc"
