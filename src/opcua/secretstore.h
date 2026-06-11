// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file secretstore.h
/// \brief Declares secure storage for connection profile secrets.
///

#pragma once

#include <QObject>
#include <QString>

///
/// \brief Asynchronous adapter for the operating system credential store.
///
class SecretStore : public QObject
{
    Q_OBJECT

public:
    enum class Secret {
        Password,
        PrivateKeyPassword
    };

    explicit SecretStore(QObject *parent = nullptr);

    bool isAvailable() const;
    void read(const QString &profileId, Secret secret);
    void write(const QString &profileId, Secret secret, const QString &value);
    void remove(const QString &profileId, Secret secret);

signals:
    void readFinished(QString profileId, Secret secret, QString value, QString error);
    void writeFinished(QString profileId, Secret secret, QString error);

private:
    QString key(const QString &profileId, Secret secret) const;
};

Q_DECLARE_METATYPE(SecretStore::Secret)
