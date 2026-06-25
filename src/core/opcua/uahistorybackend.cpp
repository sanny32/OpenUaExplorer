// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "uahistorybackend.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include <memory>

#include <open62541.h>

#include "pkimanager.h"

namespace {

struct UaByteString
{
    UA_ByteString value;

    UaByteString() { UA_ByteString_init(&value); }
    ~UaByteString() { UA_ByteString_deleteMembers(&value); }
    UaByteString(const UaByteString &) = delete;
    UaByteString &operator=(const UaByteString &) = delete;
};

struct UaByteStringArray
{
    UA_ByteString *data = nullptr;
    size_t size = 0;

    ~UaByteStringArray()
    {
        if (data)
            UA_Array_delete(data, size, &UA_TYPES[UA_TYPES_BYTESTRING]);
    }
    UaByteStringArray(const UaByteStringArray &) = delete;
    UaByteStringArray &operator=(const UaByteStringArray &) = delete;
};

struct UaClientDeleter
{
    void operator()(UA_Client *client) const
    {
        if (client)
            UA_Client_delete(client);
    }
};

struct HistoryContext
{
    QString nodeId;
    QVector<OpcUaHistoryValue> values;
    QString error;
    quint32 maxValues = 0;
};

/// \brief Translates user-facing text in the Qt OPC UA backend context.
QString backendTr(const char *text)
{
    return QCoreApplication::translate("QtOpcUaBackend", text);
}

/// \brief Returns an open62541 status name.
QString statusName(UA_StatusCode status)
{
    return QString::fromLatin1(UA_StatusCode_name(status));
}

/// \brief Copies a Qt string into an open62541 string.
UA_String uaString(const QString &text)
{
    return UA_STRING_ALLOC(text.toUtf8().constData());
}

/// \brief Converts an OPC UA DateTime value to UTC Qt time.
QDateTime dateTimeToQt(UA_DateTime dateTime)
{
    if (dateTime == 0)
        return QDateTime();
    return QDateTime::fromMSecsSinceEpoch(UA_DateTime_toUnixTime(dateTime) * 1000, Qt::UTC);
}

/// \brief Converts an OPC UA String to QString.
QString stringToQt(const UA_String &value)
{
    return QString::fromUtf8(reinterpret_cast<const char *>(value.data),
                             static_cast<int>(value.length));
}

/// \brief Reads a certificate, key or CRL file into an open62541 byte string.
bool loadFileToByteString(const QString &path, UaByteString *target, QString *error)
{
    if (!target) {
        if (error)
            *error = backendTr("Internal error while loading PKI data.");
        return false;
    }
    if (path.isEmpty()) {
        if (error)
            *error = backendTr("PKI file path is empty.");
        return false;
    }
    QFile file(path);
    if (!file.open(QFile::ReadOnly)) {
        if (error)
            *error = backendTr("Could not open PKI file %1: %2.").arg(path, file.errorString());
        return false;
    }
    QByteArray data = file.readAll();
    if (data.startsWith('-'))
        data.append('\0');
    UA_ByteString temporary;
    temporary.length = static_cast<size_t>(data.size());
    temporary.data = data.isEmpty()
        ? nullptr
        : reinterpret_cast<UA_Byte *>(data.data());
    const UA_StatusCode status = UA_ByteString_copy(&temporary, &target->value);
    if (status != UA_STATUSCODE_GOOD && error)
        *error = backendTr("Could not copy PKI file %1: %2.").arg(path, statusName(status));
    return status == UA_STATUSCODE_GOOD;
}

/// \brief Reads all files from a PKI directory into an open62541 byte string array.
bool loadDirectoryToByteStrings(const QString &path, UaByteStringArray *target, QString *error)
{
    if (!target) {
        if (error)
            *error = backendTr("Internal error while loading PKI directory data.");
        return false;
    }
    QDir directory(path);
    const QStringList entries = directory.entryList(QDir::Files);
    if (entries.isEmpty())
        return true;
    target->data = static_cast<UA_ByteString *>(
        UA_Array_new(static_cast<size_t>(entries.size()), &UA_TYPES[UA_TYPES_BYTESTRING]));
    if (!target->data) {
        if (error)
            *error = backendTr("Could not allocate PKI directory data for %1.").arg(path);
        return false;
    }
    target->size = static_cast<size_t>(entries.size());
    for (int i = 0; i < entries.size(); ++i) {
        UaByteString item;
        if (!loadFileToByteString(directory.filePath(entries.at(i)), &item, error))
            return false;
        const UA_StatusCode status = UA_ByteString_copy(&item.value, &target->data[i]);
        if (status != UA_STATUSCODE_GOOD) {
            if (error)
                *error = backendTr("Could not copy PKI directory item %1: %2.")
                    .arg(entries.at(i), statusName(status));
            return false;
        }
    }
    return true;
}

/// \brief Parses the NodeId string form produced by Qt OPC UA.
bool parseNodeId(const QString &text, UA_NodeId *nodeId, QString *error)
{
    UA_NodeId_init(nodeId);
    QString rest = text;
    quint16 namespaceIndex = 0;
    if (rest.startsWith(QLatin1String("ns="))) {
        const int separator = rest.indexOf(QLatin1Char(';'));
        bool ok = false;
        namespaceIndex = rest.mid(3, separator - 3).toUShort(&ok);
        if (!ok || separator < 0) {
            if (error)
                *error = backendTr("Unsupported NodeId format: %1.").arg(text);
            return false;
        }
        rest = rest.mid(separator + 1);
    }
    if (rest.startsWith(QLatin1String("i="))) {
        bool ok = false;
        const quint32 identifier = rest.mid(2).toUInt(&ok);
        if (!ok) {
            if (error)
                *error = backendTr("Unsupported numeric NodeId: %1.").arg(text);
            return false;
        }
        *nodeId = UA_NODEID_NUMERIC(namespaceIndex, identifier);
        return true;
    }
    if (rest.startsWith(QLatin1String("s="))) {
        *nodeId = UA_NODEID_STRING_ALLOC(namespaceIndex, rest.mid(2).toUtf8().constData());
        return true;
    }
    if (error)
        *error = backendTr("Unsupported NodeId format: %1.").arg(text);
    return false;
}

/// \brief Maps a scalar open62541 variant into a Qt value.
QVariant scalarVariantToQt(const UA_Variant &variant)
{
    if (UA_Variant_isEmpty(&variant) || !UA_Variant_isScalar(&variant))
        return QVariant();
    if (variant.type == &UA_TYPES[UA_TYPES_BOOLEAN])
        return QVariant(*static_cast<UA_Boolean *>(variant.data) != 0);
    if (variant.type == &UA_TYPES[UA_TYPES_SBYTE])
        return QVariant(static_cast<int>(*static_cast<UA_SByte *>(variant.data)));
    if (variant.type == &UA_TYPES[UA_TYPES_BYTE])
        return QVariant(static_cast<uint>(*static_cast<UA_Byte *>(variant.data)));
    if (variant.type == &UA_TYPES[UA_TYPES_INT16])
        return QVariant(static_cast<int>(*static_cast<UA_Int16 *>(variant.data)));
    if (variant.type == &UA_TYPES[UA_TYPES_UINT16])
        return QVariant(static_cast<uint>(*static_cast<UA_UInt16 *>(variant.data)));
    if (variant.type == &UA_TYPES[UA_TYPES_INT32])
        return QVariant(*static_cast<UA_Int32 *>(variant.data));
    if (variant.type == &UA_TYPES[UA_TYPES_UINT32])
        return QVariant(*static_cast<UA_UInt32 *>(variant.data));
    if (variant.type == &UA_TYPES[UA_TYPES_INT64])
        return QVariant(static_cast<qlonglong>(*static_cast<UA_Int64 *>(variant.data)));
    if (variant.type == &UA_TYPES[UA_TYPES_UINT64])
        return QVariant(static_cast<qulonglong>(*static_cast<UA_UInt64 *>(variant.data)));
    if (variant.type == &UA_TYPES[UA_TYPES_FLOAT])
        return QVariant(*static_cast<UA_Float *>(variant.data));
    if (variant.type == &UA_TYPES[UA_TYPES_DOUBLE])
        return QVariant(*static_cast<UA_Double *>(variant.data));
    if (variant.type == &UA_TYPES[UA_TYPES_STRING])
        return QVariant(stringToQt(*static_cast<UA_String *>(variant.data)));
    if (variant.type == &UA_TYPES[UA_TYPES_DATETIME])
        return QVariant(dateTimeToQt(*static_cast<UA_DateTime *>(variant.data)));
    return QVariant();
}

/// \brief Appends decoded history samples to the request context.
UA_Boolean collectHistory(UA_Client *, const UA_NodeId *, UA_Boolean moreDataAvailable,
                          const UA_ExtensionObject *data, void *callbackContext)
{
    auto *context = static_cast<HistoryContext *>(callbackContext);
    if (!data || data->encoding != UA_EXTENSIONOBJECT_DECODED
        || data->content.decoded.type != &UA_TYPES[UA_TYPES_HISTORYDATA]) {
        context->error = backendTr("History response did not contain decoded HistoryData.");
        return false;
    }
    const auto *history = static_cast<const UA_HistoryData *>(data->content.decoded.data);
    for (size_t i = 0; i < history->dataValuesSize; ++i) {
        if (context->maxValues > 0
            && context->values.size() >= static_cast<int>(context->maxValues)) {
            return false;
        }
        const UA_DataValue &sample = history->dataValues[i];
        OpcUaHistoryValue value;
        value.nodeId = context->nodeId;
        value.value = scalarVariantToQt(sample.value);
        value.status = statusName(sample.status);
        value.sourceTimestamp = dateTimeToQt(sample.sourceTimestamp);
        value.serverTimestamp = dateTimeToQt(sample.serverTimestamp);
        context->values.append(value);
    }
    return moreDataAvailable
        && (context->maxValues == 0
            || context->values.size() < static_cast<int>(context->maxValues));
}

/// \brief Initializes the open62541 client configuration from a connection profile.
bool configureClient(UA_ClientConfig *config, const ConnectionProfile &profile,
                     int timeoutMs, QString *error)
{
    const bool secureEndpoint = profile.securityMode != UA_MESSAGESECURITYMODE_NONE
        || profile.securityPolicy != QLatin1String("http://opcfoundation.org/UA/SecurityPolicy#None");
#ifdef UA_ENABLE_ENCRYPTION
    if (secureEndpoint || !profile.clientCertificateFile.isEmpty()) {
        if (profile.clientCertificateFile.isEmpty() || profile.privateKeyFile.isEmpty()) {
            if (error)
                *error = backendTr("Secure history reads require a client certificate and private key.");
            return false;
        }
        PkiManager pki;
        if (!pki.ensureDirectories(error))
            return false;
        UaByteString localCertificate;
        UaByteString privateKey;
        UaByteStringArray trustList;
        UaByteStringArray revocationList;
        const PkiManager::Paths paths = pki.paths();
        if (!loadFileToByteString(profile.clientCertificateFile, &localCertificate, error)
            || !loadFileToByteString(profile.privateKeyFile, &privateKey, error)
            || !loadDirectoryToByteStrings(paths.trustedCertificates, &trustList, error)
            || !loadDirectoryToByteStrings(paths.trustedCrl, &revocationList, error)) {
            return false;
        }
        const UA_StatusCode status = UA_ClientConfig_setDefaultEncryption(
            config, localCertificate.value, privateKey.value, trustList.data, trustList.size,
            revocationList.data, revocationList.size);
        if (status != UA_STATUSCODE_GOOD) {
            if (error)
                *error = backendTr("Could not initialize open62541 PKI: %1.").arg(statusName(status));
            return false;
        }
    } else {
        UA_ClientConfig_setDefault(config);
    }
#else
    if (secureEndpoint) {
        if (error)
            *error = backendTr("This build does not include open62541 encryption support.");
        return false;
    }
    UA_ClientConfig_setDefault(config);
#endif
    config->timeout = static_cast<UA_UInt32>(qMax(1000, timeoutMs));
    config->requestedSessionTimeout = static_cast<UA_UInt32>(qMax(1000, profile.sessionTimeoutMs));
    config->secureChannelLifeTime = static_cast<UA_UInt32>(
        qMax(1000, profile.secureChannelLifetimeMs));
    UA_String_clear(&config->securityPolicyUri);
    config->securityPolicyUri = uaString(profile.securityPolicy);
    config->securityMode = static_cast<UA_MessageSecurityMode>(profile.securityMode);
    UA_LocalizedText_clear(&config->clientDescription.applicationName);
    UA_String_clear(&config->clientDescription.applicationUri);
    UA_String_clear(&config->clientDescription.productUri);
    config->clientDescription.applicationName =
        UA_LOCALIZEDTEXT_ALLOC("", PkiManager::clientCertificateCommonName().toUtf8().constData());
    config->clientDescription.applicationUri = uaString(PkiManager::applicationUri());
    config->clientDescription.productUri =
        UA_STRING_ALLOC("https://github.com/open62541/open62541");
    config->clientDescription.applicationType = UA_APPLICATIONTYPE_CLIENT;
    return true;
}

} // namespace

///
/// \brief Reads raw historical Value samples with a short-lived open62541 session.
/// \param profile Connection profile used by the active Qt OPC UA session.
/// \param password Username password.
/// \param privateKeyPassword Private key password; encrypted keys are unsupported.
/// \param nodeId Node whose Value history is read.
/// \param start Inclusive range start.
/// \param end Inclusive range end.
/// \param maxValues Maximum samples to return, or 0 for no limit.
/// \param timeoutMs Request timeout in milliseconds.
/// \return History values or a user-facing error.
///
UaHistoryReadResult UaHistoryBackend::readRaw(
    const ConnectionProfile &profile, const QString &password,
    const QString &privateKeyPassword, const QString &nodeId, const QDateTime &start,
    const QDateTime &end, quint32 maxValues, int timeoutMs) const
{
    UaHistoryReadResult result;
    if (!privateKeyPassword.isEmpty()) {
        result.error = backendTr("Encrypted private keys are not supported by Qt OPC UA.");
        return result;
    }
    if (profile.authentication == ConnectionProfile::Authentication::Certificate) {
        result.error = backendTr("Certificate user authentication is not supported by the Qt5 open62541 backend.");
        return result;
    }
    std::unique_ptr<UA_Client, UaClientDeleter> client(UA_Client_new());
    if (!client) {
        result.error = backendTr("Could not create the open62541 history client.");
        return result;
    }
    QString error;
    if (!configureClient(UA_Client_getConfig(client.get()), profile, timeoutMs, &error)) {
        result.error = error;
        return result;
    }
    const QByteArray endpointUrl = profile.endpointUrl.toUtf8();
    UA_StatusCode status = UA_STATUSCODE_BADUNEXPECTEDERROR;
    if (profile.authentication == ConnectionProfile::Authentication::Username) {
        status = UA_Client_connect_username(client.get(), endpointUrl.constData(),
                                            profile.username.toUtf8().constData(),
                                            password.toUtf8().constData());
    } else {
        status = UA_Client_connect(client.get(), endpointUrl.constData());
    }
    if (status != UA_STATUSCODE_GOOD) {
        result.error = backendTr("History client connection failed: %1.").arg(statusName(status));
        return result;
    }
    UA_NodeId parsedNodeId;
    if (!parseNodeId(nodeId, &parsedNodeId, &error)) {
        UA_Client_disconnect(client.get());
        result.error = error;
        return result;
    }
    HistoryContext context;
    context.nodeId = nodeId;
    context.maxValues = maxValues;
    status = UA_Client_HistoryRead_raw(
        client.get(), &parsedNodeId, collectHistory,
        UA_DateTime_fromUnixTime(start.toUTC().toSecsSinceEpoch()),
        UA_DateTime_fromUnixTime(end.toUTC().toSecsSinceEpoch()),
        UA_STRING_NULL, false, maxValues, UA_TIMESTAMPSTORETURN_BOTH, &context);
    UA_NodeId_deleteMembers(&parsedNodeId);
    UA_Client_disconnect(client.get());
    if (status != UA_STATUSCODE_GOOD) {
        result.error = backendTr("History read failed: %1.").arg(statusName(status));
        return result;
    }
    result.values = context.values;
    result.error = context.error;
    return result;
}
