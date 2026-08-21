// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file imageviewdialog.h
/// \brief Declares a reusable dialog that shows an encoded picture.
///

#pragma once

#include "dialogs/appbasedialog.h"

#include <QByteArray>
#include <QImage>
#include <QPixmap>
#include <QPoint>
#include <QString>

namespace Ui {
class ImageViewDialog;
}

///
/// \brief Displays an encoded picture with copy and save actions.
///
/// OPC UA carries pictures as ByteStrings, which no table cell can show; this is the
/// viewer those cells hand their value to. The bytes are kept as they arrived so saving
/// writes the server's own file rather than a re-encoded copy of it.
///
class ImageViewDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog and wires its actions.
    /// \param parent Parent widget.
    ///
    explicit ImageViewDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~ImageViewDialog() override;

    ///
    /// \brief Sets the encoded picture shown in the view.
    /// \param data Encoded picture bytes.
    ///
    void setImageData(const QByteArray &data);

    ///
    /// \brief Sets the name the header shows above the picture's description.
    /// \param name Node or attribute the picture came from; an empty name hides the line.
    ///
    void setImageName(const QString &name);

    ///
    /// \brief Returns the decoded picture, or a null image when the bytes cannot be read.
    ///
    QImage image() const;

    ///
    /// \brief Opens an encoded picture in a viewer window of its own.
    /// \param parent Parent widget the viewer stays above.
    /// \param title Window title for the viewer.
    /// \param name Name shown in the dialog header.
    /// \param data Encoded picture bytes.
    ///
    /// The viewer is modeless, so several pictures can be compared side by side while the
    /// rest of the application keeps working. Each window owns itself and is destroyed when
    /// it is closed.
    ///
    static void showImage(QWidget *parent, const QString &title, const QString &name,
                          const QByteArray &data);

protected:
    ///
    /// \brief Refits the picture whenever the area it is shown in changes size.
    /// \param watched Object the event was sent to.
    /// \param event Event being delivered.
    /// \return False, so the scroll area keeps handling the event.
    ///
    /// The viewport reaches its real size during the first layout, which the dialog does
    /// not resize for, so watching the dialog would leave the first fit measured against
    /// nothing.
    ///
    bool eventFilter(QObject *watched, QEvent *event) override;

    ///
    /// \brief Repaints the header in the colours of the new theme.
    /// \param event Change event.
    ///
    void changeEvent(QEvent *event) override;

    ///
    /// \brief Steps the window aside from the viewers already open.
    /// \param event Show event.
    ///
    /// Modeless viewers are centred on the same parent, so without this a second picture
    /// would land exactly on the first one and look like the window that was already there.
    ///
    void showEvent(QShowEvent *event) override;

private slots:
    void copyImage();
    void saveImage();
    void fitClicked(bool checked);
    void zoomBy(int steps);
    void zoomChanged();
    void updateScaling();

private:
    ///
    /// \brief Writes the badge and the line describing the picture and its byte count.
    ///
    void updateHeader();

    ///
    /// \brief Applies the theme's caption colour to the description line.
    ///
    void applyHeaderColors();

    ///
    /// \brief Returns the zoom factor that fits the picture into the viewport.
    /// \return Factor no larger than 1.0, or 1.0 when there is nothing to measure.
    ///
    qreal fitScale() const;

    ///
    /// \brief Returns the zoom factor the picture is currently shown at.
    /// \return Fitted factor while fitting is on, otherwise the slider's own.
    ///
    qreal currentScale() const;

    ///
    /// \brief Moves the slider and its readout to a factor without re-entering the scaling.
    /// \param scale Zoom factor being shown.
    ///
    void syncZoomControls(qreal scale);

    ///
    /// \brief Returns the picture scaled to the current zoom factor.
    /// \return Pixmap to show, at its own size when the factor is 1.
    ///
    QPixmap scaledPixmap() const;

    ///
    /// \brief Returns the file filter and default suffix the picture's format calls for.
    /// \param suffix Receives the default file suffix.
    /// \return Save-dialog filter string.
    ///
    QString saveFilter(QString *suffix) const;

    ///
    /// \brief Returns how far this window steps away from the viewers already open.
    /// \return Offset to add to the position the dialog was centred at.
    ///
    QPoint cascadeOffset() const;

    Ui::ImageViewDialog *ui;
    QByteArray _data;
    QImage _image;
    /// \brief Decoded picture at its own size, rescaled on every zoom change.
    QPixmap _pixmap;
    QString _format;
    /// \brief Guards against a refit that the refit's own relayout asked for.
    bool _scaling = false;
    /// \brief Keeps a reshow from stepping the window aside a second time.
    bool _cascaded = false;
};
