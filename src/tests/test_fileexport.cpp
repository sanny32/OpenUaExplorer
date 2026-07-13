// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_fileexport.cpp
/// \brief Tests the shared file export helper.
///

#include <QDir>
#include <QFile>
#include <QStandardItemModel>
#include <QTemporaryDir>
#include <QTest>

#include "fileexport.h"
#include "models/csvexporter.h"

namespace {

///
/// \brief Reads a file back as UTF-8 with line endings normalized to "\n".
///
/// Exports are written in QIODevice::Text mode, so the on-disk line endings are
/// the platform's native ones; the tests assert on content, not on that detail.
/// \param path File to read.
/// \return File content with any "\r\n" collapsed to "\n".
///
QString readBackNormalized(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QString text = QString::fromUtf8(file.readAll());
    return text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
}

} // namespace

class TestFileExport : public QObject
{
    Q_OBJECT

private slots:
    void writesUtf8TextAndOverwrites();
    void reportsAnErrorWhenThePathIsUnwritable();
    void exportsAModelThroughTheCsvSerializer();
};

///
/// \brief Text round-trips as UTF-8, and an existing file is replaced atomically.
///
void TestFileExport::writesUtf8TextAndOverwrites()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("export.csv"));

    QVERIFY(FileExport::writeTextFile(path, QStringLiteral("first,Тест\n")));
    QCOMPARE(readBackNormalized(path), QStringLiteral("first,Тест\n"));

    QVERIFY(FileExport::writeTextFile(path, QStringLiteral("second\n")));
    QCOMPARE(readBackNormalized(path), QStringLiteral("second\n"));
}

///
/// \brief A write into a missing directory fails and yields a message naming the file.
///
void TestFileExport::reportsAnErrorWhenThePathIsUnwritable()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("no_such_dir/export.csv"));

    QString error;
    QVERIFY(!FileExport::writeTextFile(path, QStringLiteral("data\n"), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains(path));
    QVERIFY(!QFile::exists(path));
}

///
/// \brief The CSV written for a model matches the shared serializer's output.
///
void TestFileExport::exportsAModelThroughTheCsvSerializer()
{
    QStandardItemModel model(1, 2);
    model.setHorizontalHeaderLabels({QStringLiteral("Node"), QStringLiteral("Value")});
    model.setItem(0, 0, new QStandardItem(QStringLiteral("ns=2;s=Temp")));
    model.setItem(0, 1, new QStandardItem(QStringLiteral("12,5")));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("model.csv"));
    QVERIFY(FileExport::writeTextFile(path, CsvExporter::tableToCsv(model)));

    QCOMPARE(readBackNormalized(path), QStringLiteral("Node,Value\nns=2;s=Temp,\"12,5\"\n"));
}

QTEST_MAIN(TestFileExport)

#include "test_fileexport.moc"
