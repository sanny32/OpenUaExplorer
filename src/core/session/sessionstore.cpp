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

///
/// \brief Serializes one trend tab, with its display settings and series, to JSON.
/// \param tab Trend tab to serialize.
/// \return JSON object mirroring the tab fields.
///
QJsonObject trendTabToJson(const SessionTrendTab &tab)
{
    QJsonObject json;
    json[QStringLiteral("autoScale")] = tab.autoScale;
    json[QStringLiteral("showLegend")] = tab.showLegend;
    json[QStringLiteral("showGrid")] = tab.showGrid;
    json[QStringLiteral("smoothLines")] = tab.smoothLines;
    json[QStringLiteral("lineType")] = tab.lineType;
    json[QStringLiteral("showPoints")] = tab.showPoints;
    json[QStringLiteral("showValueTooltip")] = tab.showValueTooltip;
    json[QStringLiteral("labelMode")] = tab.labelMode;
    json[QStringLiteral("autoScrollLive")] = tab.autoScrollLive;
    json[QStringLiteral("liveSubscription")] = tab.liveSubscription;
    json[QStringLiteral("mode")] = tab.mode;
    json[QStringLiteral("windowMs")] = static_cast<double>(tab.windowMs);

    QJsonArray series;
    for (const SessionTrendSeries &item : tab.series) {
        QJsonObject entry;
        entry[QStringLiteral("nodeId")] = item.nodeId;
        entry[QStringLiteral("displayName")] = item.displayName;
        entry[QStringLiteral("displayPath")] = item.displayPath;
        entry[QStringLiteral("color")] = item.color;
        entry[QStringLiteral("visible")] = item.visible;
        series.append(entry);
    }
    json[QStringLiteral("series")] = series;
    return json;
}

///
/// \brief Reconstructs a trend tab from a JSON object.
/// \param json JSON object produced by trendTabToJson().
/// \return Parsed trend tab with default-backed missing fields.
///
SessionTrendTab trendTabFromJson(const QJsonObject &json)
{
    SessionTrendTab tab;
    tab.autoScale = json[QStringLiteral("autoScale")].toBool(true);
    tab.showLegend = json[QStringLiteral("showLegend")].toBool(true);
    tab.showGrid = json[QStringLiteral("showGrid")].toBool(true);
    tab.smoothLines = json[QStringLiteral("smoothLines")].toBool(true);
    tab.lineType = json[QStringLiteral("lineType")].toInt(1);
    tab.showPoints = json[QStringLiteral("showPoints")].toBool(false);
    tab.showValueTooltip = json[QStringLiteral("showValueTooltip")].toBool(true);
    tab.labelMode = json[QStringLiteral("labelMode")].toInt(0);
    tab.autoScrollLive = json[QStringLiteral("autoScrollLive")].toBool(true);
    tab.liveSubscription = json[QStringLiteral("liveSubscription")].toString(QStringLiteral("Default"));
    tab.mode = json[QStringLiteral("mode")].toInt(0);
    tab.windowMs = static_cast<qint64>(json[QStringLiteral("windowMs")].toDouble(60000));

    const QJsonArray series = json[QStringLiteral("series")].toArray();
    for (const QJsonValue &value : series) {
        const QJsonObject entry = value.toObject();
        SessionTrendSeries item;
        item.nodeId = entry[QStringLiteral("nodeId")].toString();
        item.displayName = entry[QStringLiteral("displayName")].toString();
        item.displayPath = entry[QStringLiteral("displayPath")].toString();
        item.color = entry[QStringLiteral("color")].toString();
        item.visible = entry[QStringLiteral("visible")].toBool(true);
        if (!item.nodeId.isEmpty())
            tab.series.append(item);
    }
    return tab;
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

    QJsonArray trendTabs;
    for (const SessionTrendTab &tab : data.trendTabs)
        trendTabs.append(trendTabToJson(tab));
    root[QStringLiteral("trendTabs")] = trendTabs;

    QJsonArray expanded;
    for (const QString &nodeId : data.expandedNodes)
        expanded.append(nodeId);
    root[QStringLiteral("expandedNodes")] = expanded;
    root[QStringLiteral("selectedNode")] = data.selectedNode;

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

    const QJsonArray trendTabs = root[QStringLiteral("trendTabs")].toArray();
    for (const QJsonValue &value : trendTabs)
        parsed.trendTabs.append(trendTabFromJson(value.toObject()));

    const QJsonArray expanded = root[QStringLiteral("expandedNodes")].toArray();
    for (const QJsonValue &value : expanded) {
        const QString nodeId = value.toString();
        if (!nodeId.isEmpty())
            parsed.expandedNodes.append(nodeId);
    }
    parsed.selectedNode = root[QStringLiteral("selectedNode")].toString();

    data = parsed;
    return true;
}
