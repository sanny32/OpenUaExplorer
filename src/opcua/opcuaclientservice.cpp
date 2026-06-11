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

#include "loggingcategories.h"
#include "opcuaclientservice.h"
#include "pkimanager.h"

namespace {

///
/// \brief Checks whether a QVariant contains a sequential value array.
/// \param value Value to inspect.
/// \return True for list-like values, excluding scalar strings and byte strings.
///
bool isValueArray(const QVariant &value)
{
    return value.userType() != QMetaType::QString
        && value.userType() != QMetaType::QByteArray
        && value.canConvert<QVariantList>();
}

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
    if (isValueArray(value)) {
        const QVariantList list = value.toList();
        QStringList parts;
        parts.reserve(list.size());
        for (const QVariant &entry : list)
            parts.append(displayValue(entry));
        return QStringLiteral("[%1]").arg(parts.join(QStringLiteral(", ")));
    }
    return value.toString();
}

QOpcUa::Types valueTypeForDataType(const QString &nodeId);

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
/// \brief Formats a status code with its numeric representation.
/// \param status OPC UA status code.
/// \return Status name and hexadecimal value.
///
QString statusDisplay(QOpcUa::UaStatusCode status)
{
    return QStringLiteral("%1 (0x%2)")
        .arg(statusName(status))
        .arg(static_cast<quint32>(status), 8, 16, QLatin1Char('0'));
}

///
/// \brief Formats a timestamp like UaExpert.
/// \param timestamp Timestamp to format.
/// \return Localized timestamp text.
///
QString timestampDisplay(const QDateTime &timestamp)
{
    return timestamp.isValid()
        ? timestamp.toLocalTime().toString(QStringLiteral("dd.MM.yyyy H:mm:ss.zzz"))
        : QString();
}

///
/// \brief Returns the symbolic name of an OPC UA value type.
/// \param type OPC UA value type.
/// \return Symbolic type name.
///
QString valueTypeName(QOpcUa::Types type)
{
    const char *key = QMetaEnum::fromType<QOpcUa::Types>().valueToKey(type);
    return key ? QString::fromLatin1(key) : QObject::tr("Unknown");
}

///
/// \brief Returns the symbolic name of an OPC UA node class.
/// \param nodeClass OPC UA node class.
/// \return Symbolic node class name.
///
QString nodeClassName(QOpcUa::NodeClass nodeClass)
{
    const char *key = QMetaEnum::fromType<QOpcUa::NodeClass>()
                          .valueToKey(static_cast<int>(nodeClass));
    return key ? QString::fromLatin1(key) : QString::number(static_cast<int>(nodeClass));
}

///
/// \brief Formats an OPC UA access level mask.
/// \param accessLevel Access level mask.
/// \return Set flag names separated by vertical bars.
///
QString accessLevelDisplay(quint32 accessLevel)
{
    const QMetaEnum metaEnum = QMetaEnum::fromType<QOpcUa::AccessLevelBit>();
    QStringList names;
    for (int index = 0; index < metaEnum.keyCount(); ++index) {
        const quint32 flag = static_cast<quint32>(metaEnum.value(index));
        if (flag && (accessLevel & flag) == flag)
            names.append(QString::fromLatin1(metaEnum.key(index)));
    }
    return names.isEmpty() ? QStringLiteral("None") : names.join(QStringLiteral(" | "));
}

///
/// \brief Formats an OPC UA write mask.
/// \param writeMask Write mask.
/// \return Set flag names or zero.
///
QString writeMaskDisplay(quint32 writeMask)
{
    if (!writeMask)
        return QStringLiteral("0");
    const QMetaEnum metaEnum = QMetaEnum::fromType<QOpcUa::WriteMaskBit>();
    QStringList names;
    for (int index = 0; index < metaEnum.keyCount(); ++index) {
        const quint32 flag = static_cast<quint32>(metaEnum.value(index));
        if (flag && (writeMask & flag) == flag)
            names.append(QString::fromLatin1(metaEnum.key(index)));
    }
    return names.isEmpty() ? QString::number(writeMask)
                           : names.join(QStringLiteral(" | "));
}

///
/// \brief Formats an OPC UA value rank.
/// \param valueRank Value rank.
/// \return Numeric rank with its symbolic meaning.
///
QString valueRankDisplay(int valueRank)
{
    switch (valueRank) {
    case -3: return QStringLiteral("-3 (ScalarOrOneDimension)");
    case -2: return QStringLiteral("-2 (Any)");
    case -1: return QStringLiteral("-1 (Scalar)");
    case 0: return QStringLiteral("0 (OneOrMoreDimensions)");
    case 1: return QStringLiteral("1 (OneDimension)");
    case 2: return QStringLiteral("2 (TwoDimensions)");
    default: return QString::number(valueRank);
    }
}

///
/// \brief Returns the textual name of a NodeId identifier type.
/// \param identifierType OPC UA NodeId identifier type marker.
/// \return Identifier type name.
///
QString identifierTypeName(char identifierType)
{
    switch (identifierType) {
    case 'i': return QStringLiteral("Numeric");
    case 's': return QStringLiteral("String");
    case 'g': return QStringLiteral("Guid");
    case 'b': return QStringLiteral("ByteString");
    default: return QObject::tr("Unknown");
    }
}

///
/// \brief Creates a child row.
/// \param name Row name.
/// \param displayValue Display value.
/// \return Child attribute row.
///
OpcUaNodeAttribute childAttribute(const QString &name, const QString &displayValue)
{
    OpcUaNodeAttribute child;
    child.name = name;
    child.displayValue = displayValue;
    return child;
}

///
/// \brief Adds parsed NodeId fields to an attribute row.
/// \param attribute Attribute row to populate.
/// \param nodeId OPC UA NodeId string.
///
void addNodeIdChildren(OpcUaNodeAttribute *attribute, const QString &nodeId)
{
    quint16 namespaceIndex = 0;
    QString identifier;
    char identifierType = '\0';
    if (!QOpcUa::nodeIdStringSplit(nodeId, &namespaceIndex, &identifier, &identifierType))
        return;

    attribute->children.append(
        childAttribute(QObject::tr("NamespaceIndex"), QString::number(namespaceIndex)));
    attribute->children.append(
        childAttribute(QObject::tr("IdentifierType"), identifierTypeName(identifierType)));
    attribute->children.append(
        childAttribute(QObject::tr("Identifier"), identifier));
}

///
/// \brief Formats a NodeId and adds its parsed fields.
/// \param attribute Attribute row to populate.
/// \param nodeId OPC UA NodeId string.
///
void formatNodeIdAttribute(OpcUaNodeAttribute *attribute, const QString &nodeId)
{
    attribute->displayValue = nodeId;
    addNodeIdChildren(attribute, nodeId);
}

///
/// \brief Formats a DataType NodeId using its built-in type name when possible.
/// \param attribute Attribute row to populate.
/// \param nodeId DataType NodeId.
///
void formatDataTypeAttribute(OpcUaNodeAttribute *attribute, const QString &nodeId)
{
    const QOpcUa::Types type = valueTypeForDataType(nodeId);
    attribute->displayValue = type == QOpcUa::Types::Undefined
        ? nodeId
        : valueTypeName(type);
    addNodeIdChildren(attribute, nodeId);
}

///
/// \brief Formats a QVariant as a tree value.
/// \param value Value to format.
/// \param type OPC UA value type.
/// \return Root row containing scalar or array entries.
///
OpcUaNodeAttribute valueAttribute(const QVariant &value, QOpcUa::Types type)
{
    OpcUaNodeAttribute result = childAttribute(QObject::tr("Value"), displayValue(value));
    if (!isValueArray(value))
        return result;

    const QVariantList values = value.toList();
    result.displayValue = QStringLiteral("%1 Array[%2]")
                              .arg(valueTypeName(type))
                              .arg(values.size());
    for (int index = 0; index < values.size(); ++index) {
        result.children.append(
            childAttribute(QStringLiteral("[%1]").arg(index), displayValue(values.at(index))));
    }
    return result;
}

///
/// \brief Formats an attribute value and creates expandable child rows.
/// \param attribute Attribute row to populate.
/// \param nodeAttribute OPC UA attribute identifier.
/// \param value Attribute value.
/// \param valueType Variable value type.
///
void formatAttribute(OpcUaNodeAttribute *attribute,
                     QOpcUa::NodeAttribute nodeAttribute,
                     const QVariant &value,
                     QOpcUa::Types valueType)
{
    switch (nodeAttribute) {
    case QOpcUa::NodeAttribute::NodeId:
        formatNodeIdAttribute(attribute, value.toString());
        break;
    case QOpcUa::NodeAttribute::NodeClass:
        attribute->displayValue =
            nodeClassName(static_cast<QOpcUa::NodeClass>(value.toInt()));
        break;
    case QOpcUa::NodeAttribute::BrowseName:
        if (value.canConvert<QOpcUaQualifiedName>()) {
            const QOpcUaQualifiedName name = value.value<QOpcUaQualifiedName>();
            attribute->displayValue = QStringLiteral("%1, \"%2\"")
                                          .arg(name.namespaceIndex())
                                          .arg(name.name());
        }
        break;
    case QOpcUa::NodeAttribute::DisplayName:
    case QOpcUa::NodeAttribute::Description:
    case QOpcUa::NodeAttribute::InverseName:
        if (value.canConvert<QOpcUaLocalizedText>()) {
            const QOpcUaLocalizedText text = value.value<QOpcUaLocalizedText>();
            attribute->displayValue = QStringLiteral("\"%1\", \"%2\"")
                                          .arg(text.locale(), text.text());
        }
        break;
    case QOpcUa::NodeAttribute::ValueRank:
        attribute->displayValue = valueRankDisplay(value.toInt());
        break;
    case QOpcUa::NodeAttribute::DataType:
        formatDataTypeAttribute(attribute, value.toString());
        break;
    case QOpcUa::NodeAttribute::ArrayDimensions: {
        const QVariantList dimensions = value.toList();
        attribute->displayValue =
            QStringLiteral("UInt32 Array[%1]").arg(dimensions.size());
        for (int index = 0; index < dimensions.size(); ++index) {
            attribute->children.append(
                childAttribute(QStringLiteral("[%1]").arg(index),
                               dimensions.at(index).toString()));
        }
        break;
    }
    case QOpcUa::NodeAttribute::AccessLevel:
    case QOpcUa::NodeAttribute::UserAccessLevel:
        attribute->displayValue = accessLevelDisplay(value.toUInt());
        break;
    case QOpcUa::NodeAttribute::WriteMask:
    case QOpcUa::NodeAttribute::UserWriteMask:
        attribute->displayValue = writeMaskDisplay(value.toUInt());
        break;
    default:
        attribute->displayValue = displayValue(value);
        break;
    }

    if (nodeAttribute == QOpcUa::NodeAttribute::Value) {
        attribute->displayValue = isValueArray(value)
            ? QStringLiteral("%1 Array[%2]")
                  .arg(valueTypeName(valueType))
                  .arg(value.toList().size())
            : valueTypeName(valueType);
    }
}

///
/// \brief Checks whether an attribute is defined for a node class.
/// \param attribute OPC UA attribute.
/// \param nodeClass OPC UA node class.
/// \return True if the attribute applies to the node class.
///
bool attributeAppliesToNodeClass(QOpcUa::NodeAttribute attribute,
                                 QOpcUa::NodeClass nodeClass)
{
    switch (attribute) {
    case QOpcUa::NodeAttribute::Value:
    case QOpcUa::NodeAttribute::DataType:
    case QOpcUa::NodeAttribute::ValueRank:
    case QOpcUa::NodeAttribute::ArrayDimensions:
        return nodeClass == QOpcUa::NodeClass::Variable
            || nodeClass == QOpcUa::NodeClass::VariableType;
    case QOpcUa::NodeAttribute::AccessLevel:
    case QOpcUa::NodeAttribute::UserAccessLevel:
    case QOpcUa::NodeAttribute::MinimumSamplingInterval:
    case QOpcUa::NodeAttribute::Historizing:
        return nodeClass == QOpcUa::NodeClass::Variable;
    case QOpcUa::NodeAttribute::Executable:
    case QOpcUa::NodeAttribute::UserExecutable:
        return nodeClass == QOpcUa::NodeClass::Method;
    case QOpcUa::NodeAttribute::IsAbstract:
        return nodeClass == QOpcUa::NodeClass::ObjectType
            || nodeClass == QOpcUa::NodeClass::VariableType
            || nodeClass == QOpcUa::NodeClass::ReferenceType
            || nodeClass == QOpcUa::NodeClass::DataType;
    case QOpcUa::NodeAttribute::Symmetric:
    case QOpcUa::NodeAttribute::InverseName:
        return nodeClass == QOpcUa::NodeClass::ReferenceType;
    case QOpcUa::NodeAttribute::ContainsNoLoops:
        return nodeClass == QOpcUa::NodeClass::View;
    case QOpcUa::NodeAttribute::EventNotifier:
        return nodeClass == QOpcUa::NodeClass::Object
            || nodeClass == QOpcUa::NodeClass::View;
    default:
        return true;
    }
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
            QString message = OpcUaClientService::tr(
                "Connection step %1 failed: %2")
                .arg(static_cast<int>(state->connectionStep()))
                .arg(statusName(state->errorCode()));
            if (state->errorCode() == QOpcUa::UaStatusCode::BadCertificateInvalid) {
                message += OpcUaClientService::tr(
                    "\nThe server rejected the client certificate. Add this certificate "
                    "to the server trust list and retry: %1")
                    .arg(activeClientCertificateFile);
            }
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
    OpcUaClientService *q;
    OpcUaConnectionState currentState = OpcUaConnectionState::Disconnected;
    QString error;
    QTimer connectWatchdog;
    quint64 generation = 0;
    PkiManager pki;
    QOpcUaProvider provider;
    QOpcUaClient *client = nullptr;
    QString activeBackend;
    QList<QOpcUaEndpointDescription> endpointDescriptions;
    QByteArray activeCertificate;
    QString activeClientCertificateFile;
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
}

///
/// \brief OpcUaClientService::~OpcUaClientService
///
OpcUaClientService::~OpcUaClientService()
{
    delete _d->client;
    delete _d;
}

///
/// \brief OpcUaClientService::isAvailable
/// \return True when Qt OpcUa and at least one backend are available.
///
bool OpcUaClientService::isAvailable() const
{
    return !_d->provider.availableBackends().isEmpty();
}

///
/// \brief OpcUaClientService::availableBackends
/// \return Installed Qt OPC UA backend names.
///
QStringList OpcUaClientService::availableBackends() const
{
    return _d->provider.availableBackends();
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
}

///
/// \brief OpcUaClientService::disconnectFromEndpoint
///
void OpcUaClientService::disconnectFromEndpoint()
{
    if (_d->client)
        _d->client->disconnectFromEndpoint();
}

///
/// \brief OpcUaClientService::browse
/// \param nodeId Node to browse.
///
void OpcUaClientService::browse(const QString &nodeId)
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
}

///
/// \brief OpcUaClientService::readNode
/// \param nodeId Node to read.
///
void OpcUaClientService::readNode(const QString &nodeId)
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
    connect(node, &QOpcUaNode::attributeRead, this,
            [this, node, nodeId, attributes, requestGeneration](QOpcUa::NodeAttributes) {
        if (requestGeneration != _d->generation) {
            node->deleteLater();
            return;
        }
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

        const QList<QPair<QString, QOpcUa::NodeAttribute>> fields = {
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
        emit nodeDetailsReady(details, QString());
        node->deleteLater();
    });
    if (!node->readAttributes(attributes)) {
        node->deleteLater();
        emit nodeDetailsReady({}, tr("The backend rejected the read request."));
    }
}

///
/// \brief OpcUaClientService::readValues
/// \param nodeIds Nodes whose Value attributes should be read.
///
void OpcUaClientService::readValues(const QStringList &nodeIds)
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
}

///
/// \brief OpcUaClientService::writeValue
/// \param nodeId Node to write.
/// \param value Typed value.
/// \param valueType QOpcUa::Types numeric value or Undefined.
///
void OpcUaClientService::writeValue(const QString &nodeId, const QVariant &value, int valueType)
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
}
