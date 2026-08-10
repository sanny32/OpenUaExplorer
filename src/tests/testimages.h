// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file testimages.h
/// \brief Provides encoded sample pictures for the image-value tests.
///

#pragma once

#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QSize>

namespace TestImages {

///
/// \brief Encodes a solid picture as PNG, the way a server would send one.
/// \param size Pixel dimensions of the picture.
/// \return Encoded PNG bytes, empty when encoding failed.
///
/// PNG is built into Qt Gui rather than supplied by a plugin, so the bytes are the same
/// on every machine the tests run on.
///
inline QByteArray encodedPng(const QSize &size)
{
    QImage image(size, QImage::Format_RGB32);
    image.fill(Qt::red);

    QByteArray encoded;
    QBuffer buffer(&encoded);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG"))
        return {};
    return encoded;
}

} // namespace TestImages
