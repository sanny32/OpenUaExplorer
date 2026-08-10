// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file imageformatter.cpp
/// \brief Implements recognition and summarising of OPC UA image ByteStrings.
///

#include "attributeformatter.h"

#include <QBuffer>
#include <QImageReader>
#include <QLocale>
#include <QObject>

#include <QtOpcUa/qopcuanodeids.h>

namespace OpcUaFormat {

namespace {

/// \brief Number of leading bytes shown after the summary.
constexpr qsizetype HexPrefixBytes = 8;

///
/// \brief Returns the format name a concrete image encoding carries.
/// \param encoding Encoding to name.
/// \return Format name, or an empty string for the abstract Image type.
///
QString encodingName(ImageEncoding encoding)
{
    switch (encoding) {
    case ImageEncoding::Bmp:  return QStringLiteral("BMP");
    case ImageEncoding::Gif:  return QStringLiteral("GIF");
    case ImageEncoding::Jpeg: return QStringLiteral("JPEG");
    case ImageEncoding::Png:  return QStringLiteral("PNG");
    case ImageEncoding::Any:
    case ImageEncoding::None:
        break;
    }
    return QString();
}

} // namespace

///
/// \brief Classifies a DataType NodeId as one of the standard image types.
/// \param dataTypeId DataType NodeId string.
/// \return Encoding the DataType prescribes; Any for the abstract Image type, None otherwise.
///
ImageEncoding imageEncodingForDataType(const QString &dataTypeId)
{
    switch (QOpcUa::namespace0IdFromNodeId(dataTypeId.trimmed())) {
    case QOpcUa::NodeIds::Namespace0::Image:    return ImageEncoding::Any;
    case QOpcUa::NodeIds::Namespace0::ImageBMP: return ImageEncoding::Bmp;
    case QOpcUa::NodeIds::Namespace0::ImageGIF: return ImageEncoding::Gif;
    case QOpcUa::NodeIds::Namespace0::ImageJPG: return ImageEncoding::Jpeg;
    case QOpcUa::NodeIds::Namespace0::ImagePNG: return ImageEncoding::Png;
    default:                                    return ImageEncoding::None;
    }
}

///
/// \brief Recognises a value as a picture from its declared DataType.
/// \param value Variant to inspect.
/// \param dataTypeId DataType NodeId string backing the value.
/// \return Picture data with its format and dimensions, or nullopt when the value is none.
///
/// The DataType names the format whenever it is a concrete one, so a picture stays labelled
/// on installations where the matching image plugin is missing; only the abstract Image type
/// has to ask the reader. Reading the size touches the header alone, never the pixels.
///
std::optional<ImageValueInfo> imageValue(const QVariant &value, const QString &dataTypeId)
{
    const ImageEncoding encoding = imageEncodingForDataType(dataTypeId);
    if (encoding == ImageEncoding::None || value.userType() != QMetaType::QByteArray)
        return std::nullopt;

    ImageValueInfo info;
    info.data = value.toByteArray();
    if (info.data.isEmpty())
        return std::nullopt;

    info.formatName = encodingName(encoding);

    QBuffer buffer(&info.data);
    if (buffer.open(QIODevice::ReadOnly)) {
        QImageReader reader(&buffer);
        info.size = reader.size();
        if (info.formatName.isEmpty())
            info.formatName = QString::fromLatin1(reader.format()).toUpper();
    }

    return info;
}

///
/// \brief Renders a picture as a one-line summary with a short hex prefix.
/// \param info Picture to describe.
/// \return Summary such as "PNG 640x480, 12.1 KB - 89 50 4e 47 0d 0a 1a 0a...".
///
/// The dimensions and the byte count are what a cell can usefully say about a picture, and
/// the leading bytes keep the raw value recognisable for anyone reading the wire format.
///
QString imageSummary(const ImageValueInfo &info)
{
    QString head = info.formatName.isEmpty() ? QObject::tr("Image") : info.formatName;
    if (!info.size.isEmpty()) {
        head += QStringLiteral(" %1×%2")
                    .arg(info.size.width())
                    .arg(info.size.height());
    }
    head += QStringLiteral(", %1").arg(QLocale().formattedDataSize(info.data.size()));

    const QByteArray prefix = info.data.left(HexPrefixBytes);
    QString hex = QString::fromLatin1(prefix.toHex(' '));
    if (info.data.size() > prefix.size())
        hex += QStringLiteral("…");

    return QStringLiteral("%1 · %2").arg(head, hex);
}

} // namespace OpcUaFormat
