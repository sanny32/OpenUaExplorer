// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_application.cpp
/// \brief Tests startup behavior owned by Application.
///

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QStandardPaths>
#include <QStyle>
#include <QTest>

#include "application.h"
#include "appicons.h"
#include "opcua/pkimanager.h"
#include "startuparguments.h"

///
/// \brief Verifies application startup preflight behavior.
///
class TestApplication : public QObject
{
    Q_OBJECT

private slots:
    void startupGeneratesClientCertificate();
    void applicationIconMatchesPlatform();
    void applicationOwnsInstalledStyle();
    void sessionOptionSelectsFile_data();
    void sessionOptionSelectsFile();
    void sessionArgumentsRejectAmbiguity_data();
    void sessionArgumentsRejectAmbiguity();
};

///
/// \brief Verifies Application creates the executable-named client key pair on startup.
///
void TestApplication::startupGeneratesClientCertificate()
{
    PkiManager pki;
    QString certificateFile;
    QString privateKeyFile;
    QVERIFY(pki.existingClientCertificate(&certificateFile, &privateKeyFile));

    const QString executableBaseName =
        QFileInfo(QCoreApplication::applicationFilePath()).completeBaseName();
    QCOMPARE(QFileInfo(certificateFile).completeBaseName(), executableBaseName);
    QCOMPARE(QFileInfo(privateKeyFile).completeBaseName(), executableBaseName);
}

///
/// \brief Verifies the application icon uses Linux artwork only on Linux.
///
void TestApplication::applicationIconMatchesPlatform()
{
#ifdef Q_OS_LINUX
    const QIcon expected(QStringLiteral(":/icons/linux/app.svg"));
#else
    const QIcon expected(QString(":/icons/%1/app.ico")
                             .arg(AppIcons::isDarkTheme() ? "dark" : "light"));
#endif
    const QSize size(64, 64);
    QCOMPARE(AppIcons::application().pixmap(size).toImage(), expected.pixmap(size).toImage());
}

///
/// \brief Verifies the application owns its installed style.
///
void TestApplication::applicationOwnsInstalledStyle()
{
    QCOMPARE(QApplication::style()->parent(), qApp);
}

///
/// \brief Provides supported named and positional session arguments.
///
void TestApplication::sessionOptionSelectsFile_data()
{
    QTest::addColumn<QStringList>("arguments");
    QTest::addColumn<QString>("expectedPath");

    QTest::newRow("long option")
        << QStringList{QStringLiteral("ouaexp"), QStringLiteral("--session"),
                       QStringLiteral("saved session.ouas")}
        << QStringLiteral("saved session.ouas");
    QTest::newRow("short option")
        << QStringList{QStringLiteral("ouaexp"), QStringLiteral("-s"),
                       QStringLiteral("saved.ouas")}
        << QStringLiteral("saved.ouas");
    QTest::newRow("positional")
        << QStringList{QStringLiteral("ouaexp"), QStringLiteral("saved.ouas")}
        << QStringLiteral("saved.ouas");
}

///
/// \brief Verifies named and positional forms select the requested session file.
///
void TestApplication::sessionOptionSelectsFile()
{
    QFETCH(QStringList, arguments);
    QFETCH(QString, expectedPath);

    QCommandLineParser parser;
    configureStartupArguments(parser);
    QVERIFY2(parser.parse(arguments), qPrintable(parser.errorText()));

    QString path;
    QString error;
    QVERIFY2(resolveStartupSessionFile(parser, &path, &error), qPrintable(error));
    QCOMPARE(path, expectedPath);
}

///
/// \brief Provides conflicting startup session selections.
///
void TestApplication::sessionArgumentsRejectAmbiguity_data()
{
    QTest::addColumn<QStringList>("arguments");

    QTest::newRow("named and positional")
        << QStringList{QStringLiteral("ouaexp"), QStringLiteral("--session"),
                       QStringLiteral("named.ouas"), QStringLiteral("positional.ouas")};
    QTest::newRow("multiple positional")
        << QStringList{QStringLiteral("ouaexp"), QStringLiteral("one.ouas"),
                       QStringLiteral("two.ouas")};
}

///
/// \brief Verifies startup rejects more than one selected session file.
///
void TestApplication::sessionArgumentsRejectAmbiguity()
{
    QFETCH(QStringList, arguments);

    QCommandLineParser parser;
    configureStartupArguments(parser);
    QVERIFY2(parser.parse(arguments), qPrintable(parser.errorText()));

    QString path;
    QString error;
    QVERIFY(!resolveStartupSessionFile(parser, &path, &error));
    QVERIFY(!error.isEmpty());
}

namespace {

///
/// \brief Removes the executable-named generated certificate before Application starts.
/// \param executablePath Path used to derive the executable file base.
///
void removeStartupCertificate(const QString &executablePath)
{
    const QString executableBaseName = QFileInfo(executablePath).completeBaseName();
    const PkiManager::Paths paths = PkiManager().paths();
    QFile::remove(paths.ownCertificates + QLatin1Char('/') + executableBaseName
                  + QStringLiteral(".der"));
    QFile::remove(paths.ownPrivate + QLatin1Char('/') + executableBaseName
                  + QStringLiteral(".pem"));
}

}

///
/// \brief Runs the suite under Application so startup preflight executes.
/// \param argc Argument count.
/// \param argv Argument vector.
/// \return Test exit code.
///
int main(int argc, char *argv[])
{
    QStandardPaths::setTestModeEnabled(true);
    removeStartupCertificate(QString::fromLocal8Bit(argv[0]));

    Application app(argc, argv);
    app.setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    app.setApplicationName(QStringLiteral("Application"));

    TestApplication test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_application.moc"
