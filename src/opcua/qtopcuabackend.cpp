// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file qtopcuabackend.cpp
/// \brief Implements the Qt OPC UA transport backend.
///

#include <algorithm>
#include <memory>

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QMetaEnum>
#include <QTimer>
#include <QUrl>

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

#include "attributeformatter.h"
#include "certificatetrustdecider.h"
#include "loggingcategories.h"
#include "pkimanager.h"
#include "qtopcuabackend.h"

// The transport-neutral value/attribute formatting helpers were extracted into
// OpcUaFormat (attributeformatter.h) so they can be unit tested without a live
// connection. Pull them into scope so existing unqualified calls keep working.
using namespace OpcUaFormat;


///
/// \brief Private implementation of OpcUaClientService.
///
class QtOpcUaBackend::Private
{
public:
    explicit Private(QtOpcUaBackend *owner)
        : q(owner)
        , connectWatchdog(owner)
    {
        connectWatchdog.setSingleShot(true);
        QObject::connect(&connectWatchdog, &QTimer::timeout, q, [this]() {
            setError(QtOpcUaBackend::tr("The OPC UA connection timed out."));
            if (client)
                client->disconnectFromEndpoint();
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

    void cancelRequests()
    {
        ++discoveryGeneration;
        ++browseGeneration;
        ++nodeReadGeneration;
        ++valueReadGeneration;
        ++writeGeneration;
    }

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
            setError(QtOpcUaBackend::tr(
                "The requested OPC UA backend '%1' is unavailable. Installed backends: %2")
                .arg(backend, backends.join(QStringLiteral(", "))));
            setState(OpcUaConnectionState::Unavailable);
            return false;
        }

        client = provider.createClient(selected);
        if (!client) {
            setError(QtOpcUaBackend::tr("Could not create the OPC UA backend '%1'.").arg(selected));
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
                cancelRequests();
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
                setError(QtOpcUaBackend::tr("OPC UA client error %1.")
                             .arg(static_cast<int>(clientError)));
        });
        QObject::connect(client, &QOpcUaClient::connectError, q,
                         [this](QOpcUaErrorState *state) {
            QString message = QtOpcUaBackend::tr(
                "Connection step %1 failed: %2")
                .arg(static_cast<int>(state->connectionStep()))
                .arg(statusName(state->errorCode()));
            if (state->errorCode() == QOpcUa::UaStatusCode::BadCertificateInvalid) {
                message += QtOpcUaBackend::tr(
                    "\nThe server rejected the client certificate. Add this certificate "
                    "to the server trust list and retry: %1")
                    .arg(activeClientCertificateFile);
            }
            if (state->connectionStep() == QOpcUaErrorState::ConnectionStep::CertificateValidation) {
                const CertificateTrustDecision decision = trustDecider
                    ? trustDecider->decide(activeCertificate, message)
                    : CertificateTrustDecision::Reject;
                if (decision == CertificateTrustDecision::TrustPermanently) {
                    QString trustError;
                    if (!pki.trustServerCertificate(activeCertificate, &trustError)) {
                        setError(trustError);
                        state->setIgnoreError(false);
                        return;
                    }
                }
                state->setIgnoreError(decision != CertificateTrustDecision::Reject);
            } else {
                setError(message);
            }
        });
        return true;
    }

    void configureClient(const ConnectionProfile &profile,
                         const QString &password)
    {
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
        activeClientCertificateFile = profile.clientCertificateFile;

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

    ///
    /// \brief Translates a string in the OpcUaClientService context.
    ///
    /// Lets the helpers below reuse the same translatable strings as the
    /// surrounding member functions without qualifying every call.
    ///
    static QString tr(const char *text)
    {
        return QtOpcUaBackend::tr(text);
    }

    ///
    /// \brief Returns the ordered table of node attributes shown to the user.
    ///
    static QList<QPair<QString, QOpcUa::NodeAttribute>> attributeFieldTable()
    {
        return {
            {tr("Node Id"), QOpcUa::NodeAttribute::NodeId},
            {tr("Node Class"), QOpcUa::NodeAttribute::NodeClass},
            {tr("Browse Name"), QOpcUa::NodeAttribute::BrowseName},
            {tr("Display Name"), QOpcUa::NodeAttribute::DisplayName},
            {tr("Description"), QOpcUa::NodeAttribute::Description},
            {tr("Is Abstract"), QOpcUa::NodeAttribute::IsAbstract},
            {tr("Symmetric"), QOpcUa::NodeAttribute::Symmetric},
            {tr("Inverse Name"), QOpcUa::NodeAttribute::InverseName},
            {tr("Contains No Loops"), QOpcUa::NodeAttribute::ContainsNoLoops},
            {tr("Event Notifier"), QOpcUa::NodeAttribute::EventNotifier},
            {tr("Value"), QOpcUa::NodeAttribute::Value},
            {tr("Data Type"), QOpcUa::NodeAttribute::DataType},
            {tr("Value Rank"), QOpcUa::NodeAttribute::ValueRank},
            {tr("Array Dimensions"), QOpcUa::NodeAttribute::ArrayDimensions},
            {tr("Access Level"), QOpcUa::NodeAttribute::AccessLevel},
            {tr("User Access Level"), QOpcUa::NodeAttribute::UserAccessLevel},
            {tr("Minimum Sampling Interval"),
             QOpcUa::NodeAttribute::MinimumSamplingInterval},
            {tr("Historizing"), QOpcUa::NodeAttribute::Historizing},
            {tr("Executable"), QOpcUa::NodeAttribute::Executable},
            {tr("User Executable"), QOpcUa::NodeAttribute::UserExecutable},
            {tr("Write Mask"), QOpcUa::NodeAttribute::WriteMask},
            {tr("User Write Mask"), QOpcUa::NodeAttribute::UserWriteMask},
            {tr("Role Permissions"), QOpcUa::NodeAttribute::RolePermissions},
            {tr("User Role Permissions"), QOpcUa::NodeAttribute::UserRolePermissions},
            {tr("Access Restrictions"), QOpcUa::NodeAttribute::AccessRestrictions}
        };
    }

    ///
    /// \brief Builds the displayable node details from a freshly read node.
    /// \param node Node whose attributes were read.
    /// \param nodeId NodeId of the read node.
    /// \param attributes Attribute mask that was requested.
    /// \return Populated node details.
    ///
    OpcUaNodeDetails buildNodeDetails(QOpcUaNode *node, const QString &nodeId,
                                      QOpcUa::NodeAttributes attributes) const
    {
        OpcUaNodeDetails details;
        details.nodeId = nodeId;
        details.nodeClass = node->attribute(QOpcUa::NodeAttribute::NodeClass).toInt();
        const auto nodeClass = static_cast<QOpcUa::NodeClass>(details.nodeClass);
        details.value = node->attribute(QOpcUa::NodeAttribute::Value);
        details.dataTypeId = node->attribute(QOpcUa::NodeAttribute::DataType).toString();
        details.valueType = static_cast<int>(valueTypeForDataType(details.dataTypeId));
        const auto valueType = static_cast<QOpcUa::Types>(details.valueType);
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

        const QList<QPair<QString, QOpcUa::NodeAttribute>> fields = attributeFieldTable();
        for (const auto &field : fields) {
            if (!(attributes & field.second)
                || !attributeAppliesToNodeClass(field.second, nodeClass)
                || node->attributeError(field.second)
                    == QOpcUa::UaStatusCode::BadAttributeIdInvalid) {
                continue;
            }
            const QVariant value = node->attribute(field.second);
            OpcUaNodeAttribute attribute;
            attribute.name = field.first;
            attribute.value = value;
            attribute.status = statusName(node->attributeError(field.second));
            attribute.sourceTimestamp = node->sourceTimestamp(field.second);
            attribute.serverTimestamp = node->serverTimestamp(field.second);
            formatAttribute(&attribute, field.second, value, valueType);
            if (field.second == QOpcUa::NodeAttribute::Value) {
                if (attribute.sourceTimestamp.isValid()) {
                    attribute.children.append(
                        childAttribute(tr("Source Timestamp"),
                                       timestampDisplay(attribute.sourceTimestamp)));
                }
                if (attribute.serverTimestamp.isValid()) {
                    attribute.children.append(
                        childAttribute(tr("Server Timestamp"),
                                       timestampDisplay(attribute.serverTimestamp)));
                }
                attribute.children.append(
                    childAttribute(tr("Status Code"),
                                   statusDisplay(node->attributeError(field.second))));
                attribute.children.append(valueAttribute(value, valueType));
            }
            details.attributes.append(attribute);
        }
        const QVariant displayNameValue = node->attribute(QOpcUa::NodeAttribute::DisplayName);
        if (displayNameValue.canConvert<QOpcUaLocalizedText>())
            details.displayName = displayNameValue.value<QOpcUaLocalizedText>().text();
        return details;
    }

    QtOpcUaBackend *q;
    OpcUaConnectionState currentState = OpcUaConnectionState::Disconnected;
    QString error;
    QTimer connectWatchdog;
    quint64 generation = 0;
    quint64 discoveryGeneration = 0;
    quint64 browseGeneration = 0;
    quint64 nodeReadGeneration = 0;
    quint64 valueReadGeneration = 0;
    quint64 writeGeneration = 0;
    PkiManager pki;
    QOpcUaProvider provider;
    QOpcUaClient *client = nullptr;
    QString activeBackend;
    QList<QOpcUaEndpointDescription> endpointDescriptions;
    QByteArray activeCertificate;
    QString activeClientCertificateFile;
    CertificateTrustDecider *trustDecider = nullptr;
};

///
/// \brief OpcUaClientService::OpcUaClientService
/// \param parent Parent object.
///
QtOpcUaBackend::QtOpcUaBackend(QObject *parent)
    : OpcUaBackend(parent)
    , _d(new Private(this))
{
    qRegisterMetaType<EndpointInfo>();
    qRegisterMetaType<OpcUaNodeInfo>();
    qRegisterMetaType<OpcUaNodeDetails>();
    qRegisterMetaType<OpcUaDataValue>();
}

///
/// \brief OpcUaClientService::~OpcUaClientService
///
QtOpcUaBackend::~QtOpcUaBackend()
{
    delete _d->client;
    delete _d;
}

///
/// \brief OpcUaClientService::isAvailable
/// \return True when Qt OpcUa and at least one backend are available.
///
bool QtOpcUaBackend::isAvailable() const
{
    return !_d->provider.availableBackends().isEmpty();
}

///
/// \brief OpcUaClientService::availableBackends
/// \return Installed Qt OPC UA backend names.
///
QStringList QtOpcUaBackend::availableBackends() const
{
    return _d->provider.availableBackends();
}

///
/// \brief OpcUaClientService::state
/// \return Current connection state.
///
OpcUaConnectionState QtOpcUaBackend::state() const
{
    return _d->currentState;
}

///
/// \brief OpcUaClientService::lastError
/// \return Most recent service error.
///
QString QtOpcUaBackend::lastError() const
{
    return _d->error;
}

void QtOpcUaBackend::setCertificateTrustDecider(CertificateTrustDecider *decider)
{
    _d->trustDecider = decider;
}

///
/// \brief OpcUaClientService::discoverEndpoints
/// \param url Discovery URL.
/// \param backend Preferred backend.
///
void QtOpcUaBackend::discoverEndpoints(const QString &url, const QString &backend,
                                       int timeoutMs)
{
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
    const quint64 requestGeneration = ++_d->discoveryGeneration;
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(
        _d->client, &QOpcUaClient::endpointsRequestFinished, this,
        [this, connection, requestGeneration](
            const QList<QOpcUaEndpointDescription> &result,
            QOpcUa::UaStatusCode status, const QUrl &) {
        disconnect(*connection);
        if (requestGeneration != _d->discoveryGeneration)
            return;
        ++_d->discoveryGeneration;
        QList<EndpointInfo> endpoints;
        _d->endpointDescriptions = result;
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
            : tr("Endpoint discovery failed: %1").arg(statusName(status));
        _d->setState(OpcUaConnectionState::Disconnected);
        emit endpointsDiscovered(endpoints, message);
    });
    QTimer::singleShot(qMax(1000, timeoutMs), this,
                       [this, connection, requestGeneration]() {
        if (requestGeneration != _d->discoveryGeneration)
            return;
        disconnect(*connection);
        ++_d->discoveryGeneration;
        const QString message = tr("Endpoint discovery timed out.");
        _d->setError(message);
        _d->setState(OpcUaConnectionState::Disconnected);
        emit endpointsDiscovered({}, message);
    });
    if (!_d->client->requestEndpoints(discoveryUrl)) {
        disconnect(*connection);
        ++_d->discoveryGeneration;
        const QString message = tr("The backend rejected the endpoint discovery request.");
        _d->setError(message);
        _d->setState(OpcUaConnectionState::Disconnected);
        emit endpointsDiscovered({}, message);
    }
}

///
/// \brief OpcUaClientService::connectToEndpoint
/// \param profile Connection settings.
/// \param password Username password.
/// \param privateKeyPassword Private key password.
///
void QtOpcUaBackend::connectToEndpoint(const ConnectionProfile &profile,
                                       const QString &password,
                                       const QString &privateKeyPassword)
{
    if (!_d->createClient(profile.backend))
        return;
    if (!privateKeyPassword.isEmpty()) {
        _d->setError(tr("Encrypted private keys are not supported by Qt OPC UA."));
        return;
    }

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

    _d->configureClient(profile, password);
    _d->activeCertificate = _d->endpointDescriptions.at(endpointIndex).serverCertificate();
    _d->connectWatchdog.start(qMax(1000, profile.connectTimeoutMs));
    _d->client->connectToEndpoint(_d->endpointDescriptions.at(endpointIndex));
}

///
/// \brief OpcUaClientService::disconnectFromEndpoint
///
void QtOpcUaBackend::disconnectFromEndpoint()
{
    _d->cancelRequests();
    if (_d->client)
        _d->client->disconnectFromEndpoint();
}

///
/// \brief OpcUaClientService::browse
/// \param nodeId Node to browse.
///
void QtOpcUaBackend::browse(const QString &nodeId, int timeoutMs)
{
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
    const quint64 operationGeneration = ++_d->browseGeneration;
    QTimer::singleShot(qMax(1000, timeoutMs), this,
                       [this, node, nodeId, operationGeneration]() {
        if (operationGeneration != _d->browseGeneration)
            return;
        ++_d->browseGeneration;
        node->deleteLater();
        emit browseFinished(nodeId, {}, tr("Browse request timed out."));
    });
    connect(node, &QOpcUaNode::browseFinished, this,
            [this, node, nodeId, requestGeneration, operationGeneration](
                const QList<QOpcUaReferenceDescription> &references,
                QOpcUa::UaStatusCode status) {
        QVector<OpcUaNodeInfo> children;
        QString error;
        if (requestGeneration != _d->generation
            || operationGeneration != _d->browseGeneration) {
            node->deleteLater();
            return;
        }
        ++_d->browseGeneration;
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
        ++_d->browseGeneration;
        node->deleteLater();
        emit browseFinished(nodeId, {}, tr("The backend rejected the browse request."));
    }
}

///
/// \brief OpcUaClientService::readNode
/// \param nodeId Node to read.
///
void QtOpcUaBackend::readNode(const QString &nodeId, int timeoutMs)
{
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
        QOpcUaNode::allBaseAttributes()
        | QOpcUa::NodeAttribute::IsAbstract
        | QOpcUa::NodeAttribute::Symmetric
        | QOpcUa::NodeAttribute::InverseName
        | QOpcUa::NodeAttribute::ContainsNoLoops
        | QOpcUa::NodeAttribute::EventNotifier
        | QOpcUa::NodeAttribute::Description
        | QOpcUa::NodeAttribute::Value
        | QOpcUa::NodeAttribute::DataType
        | QOpcUa::NodeAttribute::ValueRank
        | QOpcUa::NodeAttribute::ArrayDimensions
        | QOpcUa::NodeAttribute::AccessLevel
        | QOpcUa::NodeAttribute::UserAccessLevel
        | QOpcUa::NodeAttribute::MinimumSamplingInterval
        | QOpcUa::NodeAttribute::Historizing
        | QOpcUa::NodeAttribute::Executable
        | QOpcUa::NodeAttribute::UserExecutable;
    const quint64 requestGeneration = _d->generation;
    const quint64 operationGeneration = ++_d->nodeReadGeneration;
    QTimer::singleShot(qMax(1000, timeoutMs), this,
                       [this, node, operationGeneration]() {
        if (operationGeneration != _d->nodeReadGeneration)
            return;
        ++_d->nodeReadGeneration;
        node->deleteLater();
        emit nodeDetailsReady({}, tr("Node read timed out."));
    });
    connect(node, &QOpcUaNode::attributeRead, this,
            [this, node, nodeId, attributes, requestGeneration,
             operationGeneration](QOpcUa::NodeAttributes) {
        if (requestGeneration != _d->generation
            || operationGeneration != _d->nodeReadGeneration) {
            node->deleteLater();
            return;
        }
        ++_d->nodeReadGeneration;
        emit nodeDetailsReady(_d->buildNodeDetails(node, nodeId, attributes), QString());
        node->deleteLater();
    });
    if (!node->readAttributes(attributes)) {
        ++_d->nodeReadGeneration;
        node->deleteLater();
        emit nodeDetailsReady({}, tr("The backend rejected the read request."));
    }
}

///
/// \brief OpcUaClientService::readValues
/// \param nodeIds Nodes whose Value attributes should be read.
///
void QtOpcUaBackend::readValues(const QStringList &nodeIds, int timeoutMs)
{
    if (!_d->client || _d->currentState != OpcUaConnectionState::Connected) {
        emit dataValuesReady({}, tr("The OPC UA client is not connected."));
        return;
    }
    QList<QOpcUaReadItem> items;
    items.reserve(nodeIds.size());
    for (const QString &nodeId : nodeIds)
        items.append(QOpcUaReadItem(nodeId, QOpcUa::NodeAttribute::Value));
    const quint64 requestGeneration = _d->generation;
    const quint64 operationGeneration = ++_d->valueReadGeneration;
    QMetaObject::Connection connection;
    connection = connect(_d->client, &QOpcUaClient::readNodeAttributesFinished, this,
                         [this, connection, requestGeneration, operationGeneration](
                             const QList<QOpcUaReadResult> &results,
                             QOpcUa::UaStatusCode serviceResult) mutable {
        disconnect(connection);
        if (requestGeneration != _d->generation
            || operationGeneration != _d->valueReadGeneration)
            return;
        ++_d->valueReadGeneration;
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
    QTimer::singleShot(qMax(1000, timeoutMs), this,
                       [this, connection, operationGeneration]() {
        if (operationGeneration != _d->valueReadGeneration)
            return;
        disconnect(connection);
        ++_d->valueReadGeneration;
        emit dataValuesReady({}, tr("Value read timed out."));
    });
    if (!_d->client->readNodeAttributes(items)) {
        disconnect(connection);
        ++_d->valueReadGeneration;
        emit dataValuesReady({}, tr("The backend rejected the batch read request."));
    }
}

///
/// \brief OpcUaClientService::writeValue
/// \param nodeId Node to write.
/// \param value Typed value.
/// \param valueType QOpcUa::Types numeric value or Undefined.
///
void QtOpcUaBackend::writeValue(const QString &nodeId, const QVariant &value,
                                int valueType, int timeoutMs)
{
    if (!_d->client || _d->currentState != OpcUaConnectionState::Connected) {
        emit writeFinished(nodeId, false, tr("The OPC UA client is not connected."));
        return;
    }
    QOpcUaNode *node = _d->client->node(nodeId);
    if (!node) {
        emit writeFinished(nodeId, false, tr("Could not create node %1.").arg(nodeId));
        return;
    }
    const quint64 operationGeneration = ++_d->writeGeneration;
    QTimer::singleShot(qMax(1000, timeoutMs), this,
                       [this, node, nodeId, operationGeneration]() {
        if (operationGeneration != _d->writeGeneration)
            return;
        ++_d->writeGeneration;
        node->deleteLater();
        emit writeFinished(nodeId, false, tr("Write request timed out."));
    });
    connect(node, &QOpcUaNode::attributeWritten, this,
            [this, node, nodeId, operationGeneration](
                QOpcUa::NodeAttribute attribute, QOpcUa::UaStatusCode status) {
        if (attribute != QOpcUa::NodeAttribute::Value)
            return;
        if (operationGeneration != _d->writeGeneration) {
            node->deleteLater();
            return;
        }
        ++_d->writeGeneration;
        const bool success = QOpcUa::isSuccessStatus(status);
        emit writeFinished(nodeId, success,
                           success ? QString() : statusName(status));
        node->deleteLater();
    });
    const QOpcUa::Types type = static_cast<QOpcUa::Types>(valueType);
    if (!node->writeValueAttribute(value, type)) {
        ++_d->writeGeneration;
        node->deleteLater();
        emit writeFinished(nodeId, false, tr("The backend rejected the write request."));
    }
}
