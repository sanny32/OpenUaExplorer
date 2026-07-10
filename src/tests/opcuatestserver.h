// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file opcuatestserver.h
/// \brief Launches tools/opcua_test_server.py for the OPC UA integration tests.
///

#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QFile>
#include <QHash>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QString>
#include <QStringList>

///
/// \brief Child-process wrapper around the Python asyncua test server.
///
/// The server announces its endpoint and node identifiers on stdout before
/// printing READY. When Python or the asyncua package is unavailable the start
/// fails with a reason the caller is expected to feed into QSKIP, so the
/// integration tests stay CI-friendly.
///
class OpcUaTestServer
{
public:
    ///
    /// \brief Stops the server if it is still running.
    ///
    ~OpcUaTestServer() { stop(); }

    ///
    /// \brief Starts the server and waits until it announces itself.
    /// \param arguments Command-line arguments appended after the script path.
    /// \param skipReason Receives the reason the server could not be started.
    /// \return True when the server is ready and its endpoint is known.
    ///
    bool start(const QStringList &arguments, QString *skipReason)
    {
        // Prefer the interpreter the coverage/test runner pinned for us (guaranteed
        // to have asyncua installed); otherwise fall back to whatever is on PATH.
        QString python = qEnvironmentVariable("OUAEXP_TEST_PYTHON");
        if (python.isEmpty() || !QFile::exists(python)) {
            python = QStandardPaths::findExecutable(QStringLiteral("python"));
            if (python.isEmpty())
                python = QStandardPaths::findExecutable(QStringLiteral("python3"));
        }
        if (python.isEmpty()) {
            *skipReason = QStringLiteral("Python interpreter not found.");
            return false;
        }

        const QString script = QStringLiteral(OUAEXP_TEST_SERVER_SCRIPT);
        if (!QFile::exists(script)) {
            *skipReason = QStringLiteral("Test server script missing: %1").arg(script);
            return false;
        }

        // The Python server needs none of Qt's libraries, but the test runs with Qt
        // (and the bundled OpenSSL) prepended to the loader path. Inheriting that
        // breaks asyncua's native dependencies (cryptography loads the wrong
        // libcrypto), so launch the server with a clean loader path.
        QProcessEnvironment serverEnv = QProcessEnvironment::systemEnvironment();
        serverEnv.remove(QStringLiteral("LD_LIBRARY_PATH"));
        serverEnv.remove(QStringLiteral("DYLD_LIBRARY_PATH"));
        _process.setProcessEnvironment(serverEnv);

        _process.setProcessChannelMode(QProcess::MergedChannels);
        _process.start(python, QStringList{script} + arguments);
        if (!_process.waitForStarted(5000)) {
            *skipReason = QStringLiteral("Could not start the Python OPC UA server.");
            return false;
        }

        QByteArray output;
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 25000 && !output.contains("READY")) {
            if (_process.waitForReadyRead(1000))
                output += _process.readAll();
            if (_process.state() == QProcess::NotRunning) {
                output += _process.readAll();
                break;
            }
        }

        _announcements.clear();
        for (const QByteArray &line : output.split('\n')) {
            const QString text = QString::fromUtf8(line).trimmed();
            const int separator = text.indexOf(QLatin1Char(' '));
            if (separator > 0)
                _announcements.insert(text.left(separator), text.mid(separator + 1).trimmed());
        }

        if (endpoint().isEmpty() || nodeId().isEmpty()) {
            *skipReason = QStringLiteral(
                "OPC UA test server did not become ready (asyncua missing?). "
                "Interpreter: %1\nOutput:\n%2")
                .arg(python, QString::fromUtf8(output));
            return false;
        }
        return true;
    }

    ///
    /// \brief Stops the server process.
    ///
    void stop()
    {
        if (_process.state() != QProcess::NotRunning) {
            _process.kill();
            _process.waitForFinished(3000);
        }
    }

    /// \brief Returns the announced endpoint URL.
    QString endpoint() const { return _announcements.value(QStringLiteral("ENDPOINT")); }

    /// \brief Returns the identifier of the stable writable variable.
    QString nodeId() const { return _announcements.value(QStringLiteral("NODE")); }

    /// \brief Returns the identifier of the periodically changing variable.
    QString counterNodeId() const { return _announcements.value(QStringLiteral("COUNTER")); }

    /// \brief Returns the identifier of the multiply method.
    QString methodNodeId() const { return _announcements.value(QStringLiteral("METHOD")); }

    /// \brief Returns the identifier of the object owning the multiply method.
    QString objectsNodeId() const { return _announcements.value(QStringLiteral("OBJECTS")); }

    /// \brief Returns the path of the generated server certificate, if the server secured itself.
    QString serverCertificateFile() const
    {
        return _announcements.value(QStringLiteral("SERVER_CERT"));
    }

private:
    QProcess _process;
    QHash<QString, QString> _announcements;
};
