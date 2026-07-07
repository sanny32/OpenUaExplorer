// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file sessionstore.cpp
/// \brief Implements reading and writing of working-session files.
///

#include "sessionstore.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("SessionStore", text);
}

///
/// \brief Serializes a connection profile to a JSON object, excluding secrets.
/// \param profile Profile to serialize.
/// \return JSON object mirroring the profile fields.
///
QJsonObject profileToJson(const ConnectionProfile &profile)
{
    QJsonObject json;
    json[QStringLiteral("id")] = profile.id;
    json[QStringLiteral("name")] = profile.name;
    json[QStringLiteral("sessionName")] = profile.sessionName;
    json[QStringLiteral("backend")] = profile.backend;
    json[QStringLiteral("endpointUrl")] = profile.endpointUrl;
    json[QStringLiteral("securityPolicy")] = profile.securityPolicy;
    json[QStringLiteral("securityMode")] = profile.securityMode;
    json[QStringLiteral("authentication")] = static_cast<int>(profile.authentication);
    json[QStringLiteral("username")] = profile.username;
    json[QStringLiteral("clientCertificateFile")] = profile.clientCertificateFile;
    json[QStringLiteral("privateKeyFile")] = profile.privateKeyFile;
    json[QStringLiteral("sessionTimeoutMs")] = profile.sessionTimeoutMs;
    json[QStringLiteral("connectTimeoutMs")] = profile.connectTimeoutMs;
    json[QStringLiteral("secureChannelLifetimeMs")] = profile.secureChannelLifetimeMs;
    json[QStringLiteral("endpointTimeoutMs")] = profile.endpointTimeoutMs;
    json[QStringLiteral("requestTimeoutMs")] = profile.requestTimeoutMs;
    json[QStringLiteral("maxMessageSizeBytes")] = profile.maxMessageSizeBytes;
    return json;
}

///
/// \brief Reconstructs a connection profile from a JSON object.
/// \param json JSON object produced by profileToJson().
/// \return Parsed connection profile with default-backed missing fields.
///
ConnectionProfile profileFromJson(const QJsonObject &json)
{
    ConnectionProfile profile;
    profile.id = json[QStringLiteral("id")].toString();
    profile.name = json[QStringLiteral("name")].toString();
    profile.sessionName = json[QStringLiteral("sessionName")].toString();
    profile.backend = json[QStringLiteral("backend")].toString(QStringLiteral("open62541"));
    profile.endpointUrl = json[QStringLiteral("endpointUrl")].toString();
    profile.securityPolicy = json[QStringLiteral("securityPolicy")].toString();
    profile.securityMode = json[QStringLiteral("securityMode")].toInt(1);
    profile.authentication = static_cast<ConnectionProfile::Authentication>(
        json[QStringLiteral("authentication")].toInt(0));
    profile.username = json[QStringLiteral("username")].toString();
    profile.clientCertificateFile = json[QStringLiteral("clientCertificateFile")].toString();
    profile.privateKeyFile = json[QStringLiteral("privateKeyFile")].toString();
    profile.sessionTimeoutMs = json[QStringLiteral("sessionTimeoutMs")].toInt(600000);
    profile.connectTimeoutMs = json[QStringLiteral("connectTimeoutMs")].toInt(10000);
    profile.secureChannelLifetimeMs = json[QStringLiteral("secureChannelLifetimeMs")].toInt(600000);
    profile.endpointTimeoutMs = json[QStringLiteral("endpointTimeoutMs")].toInt(10000);
    profile.requestTimeoutMs = json[QStringLiteral("requestTimeoutMs")].toInt(15000);
    profile.maxMessageSizeBytes = json[QStringLiteral("maxMessageSizeBytes")].toInt(4194304);
    return profile;
}

} // namespace

///
/// \brief Writes a session to a JSON file.
/// \param path Destination file path.
/// \param data Session payload to serialize.
/// \param error Set to a human-readable message on failure.
/// \return True on success.
///
bool SessionStore::save(const QString &path, const SessionData &data, QString *error)
{
    QJsonObject root;
    root[QStringLiteral("schemaVersion")] = schemaVersion;
    root[QStringLiteral("connection")] = profileToJson(data.profile);

    QJsonArray subscriptions;
    for (const SubscriptionItem &item : data.subscriptions) {
        QJsonObject entry;
        entry[QStringLiteral("name")] = item.name;
        entry[QStringLiteral("publishingInterval")] = item.publishingInterval;
        subscriptions.append(entry);
    }
    root[QStringLiteral("subscriptions")] = subscriptions;

    QJsonArray nodes;
    for (const SessionNode &node : data.dataAccessNodes) {
        QJsonObject entry;
        entry[QStringLiteral("nodeId")] = node.nodeId;
        entry[QStringLiteral("subscription")] = node.subscriptionName;
        nodes.append(entry);
    }
    root[QStringLiteral("dataAccessNodes")] = nodes;

    QJsonArray trends;
    for (const QString &nodeId : data.trendNodes)
        trends.append(nodeId);
    root[QStringLiteral("trendNodes")] = trends;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

///
/// \brief Reads a session from a JSON file.
/// \param path Source file path.
/// \param data Destination for the parsed session payload.
/// \param error Set to a human-readable message on failure.
/// \return True on success.
///
bool SessionStore::load(const QString &path, SessionData &data, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error)
            *error = tr("The session file is not valid JSON.");
        return false;
    }

    const QJsonObject root = document.object();
    const QJsonObject connection = root[QStringLiteral("connection")].toObject();
    if (connection.isEmpty() || connection[QStringLiteral("endpointUrl")].toString().isEmpty()) {
        if (error)
            *error = tr("The session file does not describe a connection.");
        return false;
    }

    SessionData parsed;
    parsed.profile = profileFromJson(connection);

    const QJsonArray subscriptions = root[QStringLiteral("subscriptions")].toArray();
    for (const QJsonValue &value : subscriptions) {
        const QJsonObject entry = value.toObject();
        SubscriptionItem item;
        item.name = entry[QStringLiteral("name")].toString();
        item.publishingInterval = entry[QStringLiteral("publishingInterval")].toDouble(1000.0);
        if (!item.name.isEmpty())
            parsed.subscriptions.append(item);
    }

    const QJsonArray nodes = root[QStringLiteral("dataAccessNodes")].toArray();
    for (const QJsonValue &value : nodes) {
        const QJsonObject entry = value.toObject();
        SessionNode node;
        node.nodeId = entry[QStringLiteral("nodeId")].toString();
        node.subscriptionName = entry[QStringLiteral("subscription")].toString();
        if (!node.nodeId.isEmpty())
            parsed.dataAccessNodes.append(node);
    }

    const QJsonArray trends = root[QStringLiteral("trendNodes")].toArray();
    for (const QJsonValue &value : trends) {
        const QString nodeId = value.toString();
        if (!nodeId.isEmpty())
            parsed.trendNodes.append(nodeId);
    }

    data = parsed;
    return true;
}
