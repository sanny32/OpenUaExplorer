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
    ///
    /// \brief Secret kinds stored for a connection profile.
    ///
    enum class Secret {
        Password,
        PrivateKeyPassword
    };

    ///
    /// \brief Constructs the store and registers the Secret metatype.
    /// \param parent Parent object.
    ///
    explicit SecretStore(QObject *parent = nullptr);

    ///
    /// \brief Default destructor.
    ///
    ~SecretStore() override = default;

    ///
    /// \brief Reports whether the credential store is usable.
    /// \return True because QtKeychain is a required dependency.
    ///
    virtual bool isAvailable() const;

    ///
    /// \brief Asynchronously reads a profile secret; the result arrives via readFinished().
    /// \param profileId Profile identifier.
    /// \param secret Secret kind.
    ///
    virtual void read(const QString &profileId, Secret secret);

    ///
    /// \brief Asynchronously stores a profile secret; completion arrives via writeFinished().
    /// \param profileId Profile identifier.
    /// \param secret Secret kind.
    /// \param value Secret value.
    ///
    virtual void write(const QString &profileId, Secret secret, const QString &value);

    ///
    /// \brief Asynchronously deletes a profile secret; completion arrives via writeFinished().
    /// \param profileId Profile identifier.
    /// \param secret Secret kind.
    ///
    virtual void remove(const QString &profileId, Secret secret);

signals:
    ///
    /// \brief Emitted when a read completes.
    /// \param profileId Profile identifier.
    /// \param secret Secret kind.
    /// \param value Secret value, empty on error.
    /// \param error Error message, empty on success.
    ///
    void readFinished(QString profileId, Secret secret, QString value, QString error);

    ///
    /// \brief Emitted when a write or delete completes.
    /// \param profileId Profile identifier.
    /// \param secret Secret kind.
    /// \param error Error message, empty on success.
    ///
    void writeFinished(QString profileId, Secret secret, QString error);

private:
    QString key(const QString &profileId, Secret secret) const;
};

Q_DECLARE_METATYPE(SecretStore::Secret)
