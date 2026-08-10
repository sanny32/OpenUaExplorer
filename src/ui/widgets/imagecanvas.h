// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file imagecanvas.h
/// \brief Declares the widget that shows a picture over a transparency chequerboard.
///

#pragma once

#include <QPixmap>
#include <QString>
#include <QWidget>

///
/// \brief Draws a picture centred on a chequerboard, or a message when there is none.
///
/// A picture with an alpha channel would be invisible against a plain panel wherever it is
/// transparent, so the widget puts the familiar chequerboard behind it and lets the pattern
/// state what the picture leaves out. Sized to the pixmap, it scrolls inside a scroll area
/// once the picture is larger than the view.
///
class ImageCanvas : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty canvas.
    /// \param parent Parent widget.
    ///
    explicit ImageCanvas(QWidget *parent = nullptr);

    ///
    /// \brief Returns the picture currently drawn.
    ///
    QPixmap pixmap() const;

    ///
    /// \brief Sets the picture to draw.
    /// \param pixmap Picture at the size it should appear; a null pixmap clears the canvas.
    ///
    void setPixmap(const QPixmap &pixmap);

    ///
    /// \brief Returns the message shown while there is no picture.
    ///
    QString placeholderText() const;

    ///
    /// \brief Sets the message shown while there is no picture.
    /// \param text Message to show.
    ///
    void setPlaceholderText(const QString &text);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap _pixmap;
    QString _placeholder;
};
