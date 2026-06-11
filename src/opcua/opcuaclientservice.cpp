// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file opcuaclientservice.cpp
/// \brief Implements the application OPC UA client service.
///

#include <algorithm>

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QMetaEnum>
#include <QTimer>
#include <QUrl>

#ifdef OUAEXP_HAS_OPCUA
#include <QOpcUaAuthenticationInformation>
#include <QOpcUaClient>
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#include <QOpcUaConnectionSettings>
#endif
#include <QOpcUaEndpointDescription>
#include <QOpcUaErrorState>
#include <QOpcUaLocalizedText>
#include <QOpcUaNode>
#include <QOpcUaPkiConfiguration>
#include <QOpcUaProvider>
#include <QOpcUaQualifiedName>
#include <QOpcUaReadItem>
#include <QOpcUaReadResult>
#include <QOpcUaReferenceDescription>
#include <QOpcUaUserTokenPolicy>
#endif

#include "loggingcategories.h"
#include "opcuaclientservice.h"
#include "pkimanager.h"

namespace {

///
/// \brief Formats a QVariant for display without discarding its typed value.
/// \param value Value to format.
/// \return Human-readable value.
///
QString displayValue(const QVariant &value)
{
    if (!value.isValid())
        return QString();
    if (value.userType() == QMetaType::QByteArray)
        return QString::fromLatin1(value.toByteArray().toHex(' '));
    if (value.userType() == QMetaType::QDateTime)
        return value.toDateTime().toString(Qt::ISODateWithMs);
    if (value.canConvert<QVariantList>()) {
        const QVariantList list = value.toList();
        QStringList parts;
        parts.reserve(list.size());
        for (const QVariant &entry : list)
            parts.append(displayValue(entry));
        return QStringLiteral("[%1]").arg(parts.join(QStringLiteral(", ")));
    }
    return value.toString();
}

#ifdef OUAEXP_HAS_OPCUA
///
/// \brief Returns the localized security mode name.
/// \param mode OPC UA message security mode.
/// \return Display name.
///
QString securityModeName(QOpcUaEndpointDescription::MessageSecurityMode mode)
{
    switch (mode) {
    case QOpcUaEndpointDescription::None: return QObject::tr("None");
    case QOpcUaEndpointDescription::Sign: return QObject::tr("Sign");
    case QOpcUaEndpointDescription::SignAndEncrypt: return QObject::tr("Sign & Encrypt");
    default: return QObject::tr("Invalid");
    }
}

///
/// \brief Returns a display string for a status code.
/// \param status OPC UA status code.
/// \return Status name.
///
QString statusName(QOpcUa::UaStatusCode status)
{
    return QOpcUa::statusToString(status);
}

///
/// \brief Maps a namespace zero DataType NodeId to QOpcUa::Types.
/// \param nodeId OPC UA DataType NodeId.
/// \return Matching Qt OPC UA type.
///
QOpcUa::Types valueTypeForDataType(const QString &nodeId)
{
    bool ok = false;
    const int identifier = nodeId.section(QLatin1String("i="), 1).toInt(&ok);
    if (!ok || !nodeId.startsWith(QLatin1String("ns=0;")))
        return QOpcUa::Types::Undefined;
    switch (identifier) {
    case 1: return QOpcUa::Types::Boolean;
    case 2: return QOpcUa::Types::SByte;
    case 3: return QOpcUa::Types::Byte;
    case 4: return QOpcUa::Types::Int16;
    case 5: return QOpcUa::Types::UInt16;
    case 6: return QOpcUa::Types::Int32;
    case 7: return QOpcUa::Types::UInt32;
    case 8: return QOpcUa::Types::Int64;
    case 9: return QOpcUa::Types::UInt64;
    case 10: return QOpcUa::Types::Float;
    case 11: return QOpcUa::Types::Double;
    case 12: return QOpcUa::Types::String;
    case 13: return QOpcUa::Types::DateTime;
    case 14: return QOpcUa::Types::Guid;
    case 15: return QOpcUa::Types::ByteString;
    case 16: return QOpcUa::Types::XmlElement;
    case 17: return QOpcUa::Types::NodeId;
    case 18: return QOpcUa::Types::ExpandedNodeId;
    case 19: return QOpcUa::Types::StatusCode;
    case 20: return QOpcUa::Types::QualifiedName;
    case 21: return QOpcUa::Types::LocalizedText;
    case 22: return QOpcUa::Types::ExtensionObject;
    case 25: return QOpcUa::Types::DiagnosticInfo;
    default: return QOpcUa::Types::Undefined;
    }
}
#endif

} // namespace

///
/// \brief Private implementation of OpcUaClientService.
///
class OpcUaClientService::Private
{
public:
    explicit Private(OpcUaClientService *owner)
        : q(owner)
        , connectWatchdog(owner)
    {
        connectWatchdog.setSingleShot(true);
        QObject::connect(&connectWatchdog, &QTimer::timeout, q, [this]() {
            setError(OpcUaClientService::tr("The OPC UA connection timed out."));
#ifdef OUAEXP_HAS_OPCUA
            if (client)
                client->disconnectFromEndpoint();
#endif
        });
    }

    void setState(OpcUaConnectionState newState)
    {
        if (currentState == newState)
            return;
        currentState = newState;
        emit q->stateChanged(currentState);
    }

    void setError(const QString &message)
    {
        error = message;
        qCWarning(lcClient) << message;
        emit q->errorOccurred(message);
    }

#ifdef OUAEXP_HAS_OPCUA
    bool createClient(const QString &backend)
    {
        if (client && activeBackend == backend)
            return true;
        if (client) {
            client->deleteLater();
            client = nullptr;
        }

        const QStringList backends = provider.availableBackends();
        QString selected = backend;
        if (!backends.contains(selected)) {
            if (selected == QLatin1String("open62541")) {
                const auto match = std::find_if(backends.cbegin(), backends.cend(),
                                                [](const QString &name) {
                    return name.contains(QLatin1String("open62541"), Qt::CaseInsensitive);
                });
                if (match != backends.cend())
                    selected = *match;
            }
        }
        if (!backends.contains(selected)) {
            setError(OpcUaClientService::tr(
                "The requested OPC UA backend '%1' is unavailable. Installed backends: %2")
                .arg(backend, backends.join(QStringLiteral(", "))));
            setState(OpcUaConnectionState::Unavailable);
            return false;
        }

        client = provider.createClient(selected);
        if (!client) {
            setError(OpcUaClientService::tr("Could not create the OPC UA backend '%1'.").arg(selected));
            setState(OpcUaConnectionState::Unavailable);
            return false;
        }
        activeBackend = selected;

        QObject::connect(client, &QOpcUaClient::stateChanged, q,
                         [this](QOpcUaClient::ClientState clientState) {
            switch (clientState) {
            case QOpcUaClient::Disconnected:
                connectWatchdog.stop();
                ++generation;
                setState(OpcUaConnectionState::Disconnected);
                break;
            case QOpcUaClient::Connecting:
                setState(OpcUaConnectionState::Connecting);
                break;
            case QOpcUaClient::Connected:
                connectWatchdog.stop();
                ++generation;
                setState(OpcUaConnectionState::Connected);
                break;
            case QOpcUaClient::Closing:
                setState(OpcUaConnectionState::Closing);
                break;
            }
        });
        QObject::connect(client, &QOpcUaClient::errorChanged, q,
                         [this](QOpcUaClient::ClientError clientError) {
            if (clientError != QOpcUaClient::NoError)
                setError(OpcUaClientService::tr("OPC UA client error %1.")
                             .arg(static_cast<int>(clientError)));
        });
        QObject::connect(client, &QOpcUaClient::endpointsRequestFinished, q,
                         [this](const QList<QOpcUaEndpointDescription> &result,
                                QOpcUa::UaStatusCode status,
                                const QUrl &) {
            QList<EndpointInfo> endpoints;
            endpointDescriptions = result;
            if (QOpcUa::isSuccessStatus(status)) {
                endpoints.reserve(result.size());
                for (int i = 0; i < result.size(); ++i) {
                    const QOpcUaEndpointDescription &endpoint = result.at(i);
                    EndpointInfo info;
                    info.index = i;
                    info.endpointUrl = endpoint.endpointUrl();
                    info.securityPolicy = endpoint.securityPolicy();
                    info.securityMode = securityModeName(endpoint.securityMode());
                    info.securityModeValue = static_cast<int>(endpoint.securityMode());
                    info.serverCertificate = endpoint.serverCertificate();
                    for (const QOpcUaUserTokenPolicy &token : endpoint.userIdentityTokens()) {
                        info.supportsAnonymous |= token.tokenType() == QOpcUaUserTokenPolicy::Anonymous;
                        info.supportsUsername |= token.tokenType() == QOpcUaUserTokenPolicy::Username;
                        info.supportsCertificate |= token.tokenType() == QOpcUaUserTokenPolicy::Certificate;
                    }
                    endpoints.append(info);
                }
            }
            const QString message = QOpcUa::isSuccessStatus(status)
                ? QString()
                : OpcUaClientService::tr("Endpoint discovery failed: %1").arg(statusName(status));
            setState(OpcUaConnectionState::Disconnected);
            emit q->endpointsDiscovered(endpoints, message);
        });
        QObject::connect(client, &QOpcUaClient::connectError, q,
                         [this](QOpcUaErrorState *state) {
            const QString message = OpcUaClientService::tr(
                "Connection step %1 failed: %2")
                .arg(static_cast<int>(state->connectionStep()))
                .arg(statusName(state->errorCode()));
            if (state->connectionStep() == QOpcUaErrorState::ConnectionStep::CertificateValidation) {
                int decision = OpcUaClientService::RejectCertificate;
                emit q->certificateValidationRequired(activeCertificate, message, &decision);
                if (decision == OpcUaClientService::TrustCertificatePermanently) {
                    QString trustError;
                    if (!pki.trustServerCertificate(activeCertificate, &trustError)) {
                        setError(trustError);
                        decision = OpcUaClientService::RejectCertificate;
                    }
                }
                state->setIgnoreError(decision != OpcUaClientService::RejectCertificate);
            } else {
                setError(message);
            }
        });
        return true;
    }

    void configureClient(const ConnectionProfile &profile,
                         const QString &password,
                         const QString &privateKeyPassword)
    {
        Q_UNUSED(privateKeyPassword)
        QOpcUaAuthenticationInformation authentication;
        switch (profile.authentication) {
        case ConnectionProfile::Authentication::Username:
            authentication.setUsernameAuthentication(profile.username, password);
            break;
        case ConnectionProfile::Authentication::Certificate:
            authentication.setCertificateAuthentication();
            break;
        case ConnectionProfile::Authentication::Anonymous:
            authentication.setAnonymousAuthentication();
            break;
        }
        client->setAuthenticationInformation(authentication);

        QString pkiError;
        pki.ensureDirectories(&pkiError);
        const PkiManager::Paths paths = pki.paths();
        QOpcUaPkiConfiguration configuration;
        configuration.setClientCertificateFile(profile.clientCertificateFile);
        configuration.setPrivateKeyFile(profile.privateKeyFile);
        configuration.setTrustListDirectory(paths.trustedCertificates);
        configuration.setRevocationListDirectory(paths.trustedCrl);
        configuration.setIssuerListDirectory(paths.issuerCertificates);
        configuration.setIssuerRevocationListDirectory(paths.issuerCrl);
        client->setPkiConfiguration(configuration);
        if (configuration.isKeyAndCertificateFileSet())
            client->setApplicationIdentity(configuration.applicationIdentity());

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        QOpcUaConnectionSettings settings;
        settings.setSessionTimeout(std::chrono::milliseconds(profile.sessionTimeoutMs));
        settings.setConnectTimeout(std::chrono::milliseconds(profile.connectTimeoutMs));
        settings.setSecureChannelLifeTime(
            std::chrono::milliseconds(profile.secureChannelLifetimeMs));
        settings.setRequestTimeout(std::chrono::milliseconds(profile.requestTimeoutMs));
        client->setConnectionSettings(settings);
#endif
    }
#endif

    OpcUaClientService *q;
    OpcUaConnectionState currentState = OpcUaConnectionState::Disconnected;
    QString error;
    QTimer connectWatchdog;
    quint64 generation = 0;
    PkiManager pki;
#ifdef OUAEXP_HAS_OPCUA
    QOpcUaProvider provider;
    QOpcUaClient *client = nullptr;
    QString activeBackend;
    QList<QOpcUaEndpointDescription> endpointDescriptions;
    QByteArray activeCertificate;
#endif
};

///
/// \brief OpcUaClientService::OpcUaClientService
/// \param parent Parent object.
///
OpcUaClientService::OpcUaClientService(QObject *parent)
    : QObject(parent)
    , _d(new Private(this))
{
    qRegisterMetaType<EndpointInfo>();
    qRegisterMetaType<OpcUaNodeInfo>();
    qRegisterMetaType<OpcUaNodeDetails>();
    qRegisterMetaType<OpcUaDataValue>();
#ifndef OUAEXP_HAS_OPCUA
    _d->currentState = OpcUaConnectionState::Unavailable;
    _d->error = tr("Qt OpcUa support is not available in this build.");
#endif
}

///
/// \brief OpcUaClientService::~OpcUaClientService
///
OpcUaClientService::~OpcUaClientService()
{
#ifdef OUAEXP_HAS_OPCUA
    delete _d->client;
#endif
    delete _d;
}

///
/// \brief OpcUaClientService::isAvailable
/// \return True when Qt OpcUa and at least one backend are available.
///
bool OpcUaClientService::isAvailable() const
{
#ifdef OUAEXP_HAS_OPCUA
    return !_d->provider.availableBackends().isEmpty();
#else
    return false;
#endif
}

///
/// \brief OpcUaClientService::availableBackends
/// \return Installed Qt OPC UA backend names.
///
QStringList OpcUaClientService::availableBackends() const
{
#ifdef OUAEXP_HAS_OPCUA
    return _d->provider.availableBackends();
#else
    return {};
#endif
}

///
/// \brief OpcUaClientService::state
/// \return Current connection state.
///
OpcUaConnectionState OpcUaClientService::state() const
{
    return _d->currentState;
}

///
/// \brief OpcUaClientService::lastError
/// \return Most recent service error.
///
QString OpcUaClientService::lastError() const
{
    return _d->error;
}

///
/// \brief OpcUaClientService::discoverEndpoints
/// \param url Discovery URL.
/// \param backend Preferred backend.
///
void OpcUaClientService::discoverEndpoints(const QString &url, const QString &backend)
{
#ifdef OUAEXP_HAS_OPCUA
    if (!_d->createClient(backend)) {
        emit endpointsDiscovered({}, _d->error);
        return;
    }
    const QUrl discoveryUrl(url);
    if (!discoveryUrl.isValid() || discoveryUrl.scheme() != QLatin1String("opc.tcp")) {
        const QString message = tr("Invalid OPC UA endpoint URL: %1").arg(url);
        _d->setError(message);
        emit endpointsDiscovered({}, message);
        return;
    }
    _d->setState(OpcUaConnectionState::Discovering);
    if (!_d->client->requestEndpoints(discoveryUrl)) {
        const QString message = tr("The backend rejected the endpoint discovery request.");
        _d->setError(message);
        _d->setState(OpcUaConnectionState::Disconnected);
        emit endpointsDiscovered({}, message);
    }
#else
    Q_UNUSED(url)
    Q_UNUSED(backend)
    emit endpointsDiscovered({}, _d->error);
#endif
}

///
/// \brief OpcUaClientService::connectToEndpoint
/// \param profile Connection settings.
/// \param password Username password.
/// \param privateKeyPassword Private key password.
///
void OpcUaClientService::connectToEndpoint(const ConnectionProfile &profile,
                                           const QString &password,
                                           const QString &privateKeyPassword)
{
#ifdef OUAEXP_HAS_OPCUA
    if (!_d->createClient(profile.backend))
        return;

    int endpointIndex = -1;
    for (int i = 0; i < _d->endpointDescriptions.size(); ++i) {
        const QOpcUaEndpointDescription &candidate = _d->endpointDescriptions.at(i);
        if (candidate.endpointUrl() == profile.endpointUrl
            && candidate.securityPolicy() == profile.securityPolicy
            && static_cast<int>(candidate.securityMode()) == profile.securityMode) {
            endpointIndex = i;
            break;
        }
    }
    if (endpointIndex < 0) {
        _d->setError(tr("The selected endpoint is no longer available. Run discovery again."));
        return;
    }

    _d->configureClient(profile, password, privateKeyPassword);
    _d->activeCertificate = _d->endpointDescriptions.at(endpointIndex).serverCertificate();
    _d->connectWatchdog.start(qMax(1000, profile.connectTimeoutMs));
    _d->client->connectToEndpoint(_d->endpointDescriptions.at(endpointIndex));
#else
    Q_UNUSED(profile)
    Q_UNUSED(password)
    Q_UNUSED(privateKeyPassword)
    _d->setError(_d->error);
#endif
}

///
/// \brief OpcUaClientService::disconnectFromEndpoint
///
void OpcUaClientService::disconnectFromEndpoint()
{
#ifdef OUAEXP_HAS_OPCUA
    if (_d->client)
        _d->client->disconnectFromEndpoint();
#endif
}

///
/// \brief OpcUaClientService::browse
/// \param nodeId Node to browse.
///
void OpcUaClientService::browse(const QString &nodeId)
{
#ifdef OUAEXP_HAS_OPCUA
    if (!_d->client || _d->currentState != OpcUaConnectionState::Connected) {
        emit browseFinished(nodeId, {}, tr("The OPC UA client is not connected."));
        return;
    }
    QOpcUaNode *node = _d->client->node(nodeId);
    if (!node) {
        emit browseFinished(nodeId, {}, tr("Could not create node %1.").arg(nodeId));
        return;
    }
    const quint64 requestGeneration = _d->generation;
    connect(node, &QOpcUaNode::browseFinished, this,
            [this, node, nodeId, requestGeneration](
                const QList<QOpcUaReferenceDescription> &references,
                QOpcUa::UaStatusCode status) {
        QVector<OpcUaNodeInfo> children;
        QString error;
        if (requestGeneration != _d->generation) {
            node->deleteLater();
            return;
        }
        if (QOpcUa::isSuccessStatus(status)) {
            children.reserve(references.size());
            for (const QOpcUaReferenceDescription &reference : references) {
                OpcUaNodeInfo info;
                info.nodeId = reference.targetNodeId().nodeId();
                info.browseName = reference.browseName().name();
                info.displayName = reference.displayName().text();
                info.referenceTypeId = reference.refTypeId();
                info.nodeClass = static_cast<int>(reference.nodeClass());
                children.append(info);
            }
        } else {
            error = tr("Browse failed for %1: %2").arg(nodeId, statusName(status));
        }
        emit browseFinished(nodeId, children, error);
        node->deleteLater();
    });
    if (!node->browseChildren()) {
        node->deleteLater();
        emit browseFinished(nodeId, {}, tr("The backend rejected the browse request."));
    }
#else
    emit browseFinished(nodeId, {}, _d->error);
#endif
}

///
/// \brief OpcUaClientService::readNode
/// \param nodeId Node to read.
///
void OpcUaClientService::readNode(const QString &nodeId)
{
#ifdef OUAEXP_HAS_OPCUA
    if (!_d->client || _d->currentState != OpcUaConnectionState::Connected) {
        emit nodeDetailsReady({}, tr("The OPC UA client is not connected."));
        return;
    }
    QOpcUaNode *node = _d->client->node(nodeId);
    if (!node) {
        emit nodeDetailsReady({}, tr("Could not create node %1.").arg(nodeId));
        return;
    }
    const QOpcUa::NodeAttributes attributes =
        QOpcUaNode::mandatoryBaseAttributes()
        | QOpcUa::NodeAttribute::Description
        | QOpcUa::NodeAttribute::Value
        | QOpcUa::NodeAttribute::DataType
        | QOpcUa::NodeAttribute::ValueRank
        | QOpcUa::NodeAttribute::ArrayDimensions
        | QOpcUa::NodeAttribute::AccessLevel
        | QOpcUa::NodeAttribute::UserAccessLevel
        | QOpcUa::NodeAttribute::Historizing
        | QOpcUa::NodeAttribute::Executable
        | QOpcUa::NodeAttribute::UserExecutable;
    const quint64 requestGeneration = _d->generation;
    connect(node, &QOpcUaNode::attributeRead, this,
            [this, node, nodeId, attributes, requestGeneration](QOpcUa::NodeAttributes) {
        if (requestGeneration != _d->generation) {
            node->deleteLater();
            return;
        }
        OpcUaNodeDetails details;
        details.nodeId = nodeId;
        details.nodeClass = node->attribute(QOpcUa::NodeAttribute::NodeClass).toInt();
        details.value = node->attribute(QOpcUa::NodeAttribute::Value);
        details.dataTypeId = node->attribute(QOpcUa::NodeAttribute::DataType).toString();
        details.valueType = static_cast<int>(valueTypeForDataType(details.dataTypeId));
        details.valueRank = node->attribute(QOpcUa::NodeAttribute::ValueRank).toInt();
        const QVariant dimensions = node->attribute(QOpcUa::NodeAttribute::ArrayDimensions);
        for (const QVariant &dimension : dimensions.toList())
            details.arrayDimensions.append(dimension.toUInt());
        details.accessLevel = static_cast<quint8>(
            node->attribute(QOpcUa::NodeAttribute::AccessLevel).toUInt());
        details.userAccessLevel = static_cast<quint8>(
            node->attribute(QOpcUa::NodeAttribute::UserAccessLevel).toUInt());
        details.status = statusName(node->attributeError(QOpcUa::NodeAttribute::Value));
        details.sourceTimestamp = node->sourceTimestamp(QOpcUa::NodeAttribute::Value);
        details.serverTimestamp = node->serverTimestamp(QOpcUa::NodeAttribute::Value);

        const QList<QPair<QString, QOpcUa::NodeAttribute>> fields = {
            {tr("Node Id"), QOpcUa::NodeAttribute::NodeId},
            {tr("Node Class"), QOpcUa::NodeAttribute::NodeClass},
            {tr("Browse Name"), QOpcUa::NodeAttribute::BrowseName},
            {tr("Display Name"), QOpcUa::NodeAttribute::DisplayName},
            {tr("Description"), QOpcUa::NodeAttribute::Description},
            {tr("Value"), QOpcUa::NodeAttribute::Value},
            {tr("Data Type"), QOpcUa::NodeAttribute::DataType},
            {tr("Value Rank"), QOpcUa::NodeAttribute::ValueRank},
            {tr("Array Dimensions"), QOpcUa::NodeAttribute::ArrayDimensions},
            {tr("Access Level"), QOpcUa::NodeAttribute::AccessLevel},
            {tr("User Access Level"), QOpcUa::NodeAttribute::UserAccessLevel},
            {tr("Historizing"), QOpcUa::NodeAttribute::Historizing},
            {tr("Executable"), QOpcUa::NodeAttribute::Executable},
            {tr("User Executable"), QOpcUa::NodeAttribute::UserExecutable}
        };
        for (const auto &field : fields) {
            if (!(attributes & field.second))
                continue;
            const QVariant value = node->attribute(field.second);
            OpcUaNodeAttribute attribute;
            attribute.name = field.first;
            attribute.value = value;
            attribute.displayValue = displayValue(value);
            attribute.status = statusName(node->attributeError(field.second));
            attribute.sourceTimestamp = node->sourceTimestamp(field.second);
            attribute.serverTimestamp = node->serverTimestamp(field.second);
            details.attributes.append(attribute);
        }
        const QVariant displayNameValue = node->attribute(QOpcUa::NodeAttribute::DisplayName);
        if (displayNameValue.canConvert<QOpcUaLocalizedText>())
            details.displayName = displayNameValue.value<QOpcUaLocalizedText>().text();
        emit nodeDetailsReady(details, QString());
        node->deleteLater();
    });
    if (!node->readAttributes(attributes)) {
        node->deleteLater();
        emit nodeDetailsReady({}, tr("The backend rejected the read request."));
    }
#else
    Q_UNUSED(nodeId)
    emit nodeDetailsReady({}, _d->error);
#endif
}

///
/// \brief OpcUaClientService::readValues
/// \param nodeIds Nodes whose Value attributes should be read.
///
void OpcUaClientService::readValues(const QStringList &nodeIds)
{
#ifdef OUAEXP_HAS_OPCUA
    if (!_d->client || _d->currentState != OpcUaConnectionState::Connected) {
        emit dataValuesReady({}, tr("The OPC UA client is not connected."));
        return;
    }
    QList<QOpcUaReadItem> items;
    items.reserve(nodeIds.size());
    for (const QString &nodeId : nodeIds)
        items.append(QOpcUaReadItem(nodeId, QOpcUa::NodeAttribute::Value));
    const quint64 requestGeneration = _d->generation;
    QMetaObject::Connection connection;
    connection = connect(_d->client, &QOpcUaClient::readNodeAttributesFinished, this,
                         [this, connection, requestGeneration](
                             const QList<QOpcUaReadResult> &results,
                             QOpcUa::UaStatusCode serviceResult) mutable {
        disconnect(connection);
        if (requestGeneration != _d->generation)
            return;
        QVector<OpcUaDataValue> values;
        values.reserve(results.size());
        for (const QOpcUaReadResult &result : results) {
            OpcUaDataValue value;
            value.nodeId = result.nodeId();
            value.value = result.value();
            value.status = statusName(result.statusCode());
            value.sourceTimestamp = result.sourceTimestamp();
            value.serverTimestamp = result.serverTimestamp();
            values.append(value);
        }
        const QString error = QOpcUa::isSuccessStatus(serviceResult)
            ? QString()
            : tr("Read service failed: %1").arg(statusName(serviceResult));
        emit dataValuesReady(values, error);
    });
    if (!_d->client->readNodeAttributes(items)) {
        disconnect(connection);
        emit dataValuesReady({}, tr("The backend rejected the batch read request."));
    }
#else
    Q_UNUSED(nodeIds)
    emit dataValuesReady({}, _d->error);
#endif
}

///
/// \brief OpcUaClientService::writeValue
/// \param nodeId Node to write.
/// \param value Typed value.
/// \param valueType QOpcUa::Types numeric value or Undefined.
///
void OpcUaClientService::writeValue(const QString &nodeId, const QVariant &value, int valueType)
{
#ifdef OUAEXP_HAS_OPCUA
    if (!_d->client || _d->currentState != OpcUaConnectionState::Connected) {
        emit writeFinished(nodeId, false, tr("The OPC UA client is not connected."));
        return;
    }
    QOpcUaNode *node = _d->client->node(nodeId);
    if (!node) {
        emit writeFinished(nodeId, false, tr("Could not create node %1.").arg(nodeId));
        return;
    }
    connect(node, &QOpcUaNode::attributeWritten, this,
            [this, node, nodeId](QOpcUa::NodeAttribute attribute, QOpcUa::UaStatusCode status) {
        if (attribute != QOpcUa::NodeAttribute::Value)
            return;
        const bool success = QOpcUa::isSuccessStatus(status);
        emit writeFinished(nodeId, success,
                           success ? QString() : statusName(status));
        node->deleteLater();
    });
    const QOpcUa::Types type = static_cast<QOpcUa::Types>(valueType);
    if (!node->writeValueAttribute(value, type)) {
        node->deleteLater();
        emit writeFinished(nodeId, false, tr("The backend rejected the write request."));
    }
#else
    Q_UNUSED(value)
    Q_UNUSED(valueType)
    emit writeFinished(nodeId, false, _d->error);
#endif
}
