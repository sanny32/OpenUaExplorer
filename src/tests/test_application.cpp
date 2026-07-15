// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_application.cpp
/// \brief Tests startup behavior owned by Application.
///

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QStandardPaths>
#include <QTest>

#include "application.h"
#include "appicons.h"
#include "opcua/pkimanager.h"

///
/// \brief Verifies application startup preflight behavior.
///
class TestApplication : public QObject
{
    Q_OBJECT

private slots:
    void startupGeneratesClientCertificate();
    void applicationIconMatchesPlatform();
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
