// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file imageviewdialog.cpp
/// \brief Implements the reusable encoded-picture viewer dialog.
///

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QImageReader>
#include <QtMath>
#include <QLocale>
#include <QPixmap>
#include <QEvent>
#include <QPalette>
#include <QScopedValueRollback>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSlider>
#include <QToolButton>

#include "appcolors.h"
#include "imageviewdialog.h"
#include "messageboxdialog.h"
#include "ui_imageviewdialog.h"

namespace {

/// \brief Space the scroll area keeps free so a fitted picture never triggers scroll bars.
constexpr int ViewportPadding = 4;

/// \brief Slider units per unit of zoom, the slider counting in percent.
constexpr qreal ZoomPerCent = 100.0;

/// \brief Pixels each further viewer is moved down and to the right of the previous one.
constexpr int CascadeStep = 28;

/// \brief Viewers the cascade takes before it starts again at the centre.
constexpr int CascadeLength = 8;

} // namespace

///
/// \brief Builds the dialog and wires its actions.
/// \param parent Parent widget.
///
ImageViewDialog::ImageViewDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::ImageViewDialog)
{
    ui->setupUi(this);

    ui->closeButton->setColors(
        { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });
    applyHeaderColors();
    ui->scrollArea->viewport()->installEventFilter(this);

    connect(ui->fitButton, &QPushButton::toggled, this, &ImageViewDialog::updateScaling);
    connect(ui->fitButton, &QPushButton::clicked, this, &ImageViewDialog::fitClicked);
    connect(ui->zoomSlider, &QSlider::valueChanged, this, &ImageViewDialog::zoomChanged);
    connect(ui->zoomOutButton, &QToolButton::clicked, this, [this] { zoomBy(-1); });
    connect(ui->zoomInButton, &QToolButton::clicked, this, [this] { zoomBy(1); });
    connect(ui->copyButton, &QPushButton::clicked, this, &ImageViewDialog::copyImage);
    connect(ui->saveButton, &QPushButton::clicked, this, &ImageViewDialog::saveImage);
    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::accept);

    setImageName(QString());
    setImageData({});
}

///
/// \brief Destroys the dialog and its generated UI.
///
ImageViewDialog::~ImageViewDialog()
{
    delete ui;
}

///
/// \brief Sets the encoded picture shown in the view.
/// \param data Encoded picture bytes.
///
/// The format is read from the bytes rather than assumed, so a server that labels a JPEG
/// as ImagePNG still saves under the suffix the file actually has.
///
void ImageViewDialog::setImageData(const QByteArray &data)
{
    _data = data;
    _image = QImage();
    _format.clear();

    if (!_data.isEmpty()) {
        QBuffer buffer(&_data);
        if (buffer.open(QIODevice::ReadOnly)) {
            QImageReader reader(&buffer);
            _format = QString::fromLatin1(reader.format());
            _image = reader.read();
        }
    }
    _pixmap = QPixmap::fromImage(_image);

    const bool decoded = !_image.isNull();
    ui->copyButton->setEnabled(decoded);
    ui->fitButton->setEnabled(decoded);
    ui->zoomSlider->setEnabled(decoded);
    ui->zoomInButton->setEnabled(decoded);
    ui->zoomOutButton->setEnabled(decoded);
    ui->saveButton->setEnabled(!_data.isEmpty());

    updateHeader();
    updateScaling();
}

///
/// \brief Sets the name the header shows above the picture's description.
/// \param name Node or attribute the picture came from; an empty name hides the line.
///
void ImageViewDialog::setImageName(const QString &name)
{
    ui->nameLabel->setText(name);
    ui->nameLabel->setVisible(!name.isEmpty());
}

///
/// \brief Returns the decoded picture, or a null image when the bytes cannot be read.
///
QImage ImageViewDialog::image() const
{
    return _image;
}

///
/// \brief Opens an encoded picture in a viewer window of its own.
/// \param parent Parent widget the viewer stays above.
/// \param title Window title for the viewer.
/// \param name Name shown in the dialog header.
/// \param data Encoded picture bytes.
///
/// Pictures are worth comparing, so the viewer does not block the window it came from and
/// every call opens another one. The dialog keeps its parent, which is what holds it above
/// the main window and takes it down when that window goes, and deletes itself on close.
///
void ImageViewDialog::showImage(QWidget *parent, const QString &title, const QString &name,
                                const QByteArray &data)
{
    auto *dialog = new ImageViewDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(title);
    dialog->setImageName(name);
    dialog->setImageData(data);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

///
/// \brief Refits the picture whenever the area it is shown in changes size.
/// \param watched Object the event was sent to.
/// \param event Event being delivered.
/// \return False, so the scroll area keeps handling the event.
///
bool ImageViewDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->scrollArea->viewport() && event->type() == QEvent::Resize
        && ui->fitButton->isChecked()) {
        updateScaling();
    }
    return AppBaseDialog::eventFilter(watched, event);
}

///
/// \brief Repaints the header in the colours of the new theme.
/// \param event Change event.
///
void ImageViewDialog::changeEvent(QEvent *event)
{
    AppBaseDialog::changeEvent(event);
    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange) {
        applyHeaderColors();
    }
}

///
/// \brief Steps the window aside from the viewers already open.
/// \param event Show event.
///
/// The dialog is centred on its parent as it becomes visible, so the offset is applied on
/// top of the position it has been given by then, and only the first time: reshowing a
/// viewer should leave it where the user put it.
///
void ImageViewDialog::showEvent(QShowEvent *event)
{
    AppBaseDialog::showEvent(event);

    if (_cascaded)
        return;
    _cascaded = true;

    const QPoint offset = cascadeOffset();
    if (!offset.isNull())
        move(pos() + offset);
}

///
/// \brief Copies the decoded picture to the clipboard.
///
void ImageViewDialog::copyImage()
{
    if (_image.isNull())
        return;
    QApplication::clipboard()->setImage(_image);
}

///
/// \brief Writes the picture to a file, byte for byte as the server sent it.
///
void ImageViewDialog::saveImage()
{
    if (_data.isEmpty())
        return;

    QString suffix;
    const QString filter = saveFilter(&suffix);
    QString path = QFileDialog::getSaveFileName(this, tr("Save Image"),
                                                QStringLiteral("image.%1").arg(suffix), filter);
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(_data) != _data.size()) {
        MessageBoxDialog::warning(this, tr("Save Image"),
                                  tr("The image could not be written to %1.").arg(path),
                                  DialogButtonBox::Ok);
    }
}

///
/// \brief Returns the picture to its natural size when fitting is switched off by hand.
/// \param checked New state of the fit button.
///
/// Switching fitting off has to leave the picture at some size, and the fitted one is not a
/// meaningful place to stop, so the natural size is what the button falls back to. Only a
/// click does this: the slider and the zoom buttons clear the button too, and there the
/// factor the user just asked for is the one to keep.
///
void ImageViewDialog::fitClicked(bool checked)
{
    if (!checked)
        ui->zoomSlider->setValue(qRound(ZoomPerCent));
}

///
/// \brief Steps the zoom by whole slider pages.
/// \param steps Number of pages to add; negative zooms out.
///
void ImageViewDialog::zoomBy(int steps)
{
    ui->zoomSlider->setValue(ui->zoomSlider->value() + steps * ui->zoomSlider->pageStep());
}

///
/// \brief Switches to manual zooming and redraws at the slider's factor.
///
/// Dragging the slider is an instruction to show the picture at that size, which fitting
/// would immediately override, so fitting steps aside.
///
void ImageViewDialog::zoomChanged()
{
    if (ui->fitButton->isChecked()) {
        ui->fitButton->setChecked(false);
        return;
    }
    updateScaling();
}

///
/// \brief Redraws the picture at the size the zoom setting calls for.
///
/// A fitted picture never overflows, so its scroll bars are turned off outright: left on
/// demand, one appearing would shrink the viewport, which would refit smaller, which would
/// take the bar away again.
///
void ImageViewDialog::updateScaling()
{
    if (_scaling)
        return;
    const QScopedValueRollback<bool> guard(_scaling, true);

    const bool fitting = ui->fitButton->isChecked();
    const auto policy = fitting ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded;
    ui->scrollArea->setHorizontalScrollBarPolicy(policy);
    ui->scrollArea->setVerticalScrollBarPolicy(policy);

    if (_pixmap.isNull()) {
        ui->imageCanvas->setPixmap(QPixmap());
        ui->imageCanvas->setPlaceholderText(_data.isEmpty()
                                                ? tr("No image to show.")
                                                : tr("The image could not be decoded."));
        ui->zoomLabel->clear();
        return;
    }

    ui->imageCanvas->setPixmap(scaledPixmap());
    syncZoomControls(currentScale());
}

///
/// \brief Returns the zoom factor that fits the picture into the viewport.
/// \return Factor filling the viewport along its tighter axis, or 1.0 with nothing to measure.
///
/// Fitting magnifies as readily as it shrinks: OPC UA pictures are often icon-sized, and a
/// stamp in the middle of an empty window is not what asking to fit one means. The factor is
/// rounded to whole percent and kept inside the slider's range so the readout states exactly
/// what is on screen.
///
qreal ImageViewDialog::fitScale() const
{
    const QSize available = ui->scrollArea->viewport()->size()
                            - QSize(ViewportPadding, ViewportPadding);
    if (_pixmap.isNull() || available.width() <= 0 || available.height() <= 0)
        return 1.0;

    const qreal factor = qMin(qreal(available.width()) / _pixmap.width(),
                              qreal(available.height()) / _pixmap.height());
    const int percent = qBound(ui->zoomSlider->minimum(),
                               qFloor(factor * ZoomPerCent),
                               ui->zoomSlider->maximum());
    return percent / ZoomPerCent;
}

///
/// \brief Returns the zoom factor the picture is currently shown at.
/// \return Fitted factor while fitting is on, otherwise the slider's own.
///
qreal ImageViewDialog::currentScale() const
{
    return ui->fitButton->isChecked() ? fitScale() : ui->zoomSlider->value() / ZoomPerCent;
}

///
/// \brief Moves the slider and its readout to a factor without re-entering the scaling.
/// \param scale Zoom factor being shown.
///
void ImageViewDialog::syncZoomControls(qreal scale)
{
    const int percent = qBound(ui->zoomSlider->minimum(),
                               qRound(scale * ZoomPerCent),
                               ui->zoomSlider->maximum());
    const QSignalBlocker blocker(ui->zoomSlider);
    ui->zoomSlider->setValue(percent);
    ui->zoomLabel->setText(tr("%1%").arg(percent));
}

///
/// \brief Writes the badge and the line describing the picture and its byte count.
///
void ImageViewDialog::updateHeader()
{
    if (_data.isEmpty()) {
        ui->formatBadge->setText(QString());
        ui->infoLabel->clear();
        return;
    }

    const QString size = QLocale().formattedDataSize(_data.size());
    const bool known = !_format.isEmpty();
    ui->formatBadge->setText(known ? _format.toUpper() : QStringLiteral("?"));

    const QString format = known ? _format.toUpper() : tr("Unknown format");
    if (_image.isNull()) {
        ui->infoLabel->setText(QStringLiteral("%1 · %2").arg(format, size));
        return;
    }
    ui->infoLabel->setText(QStringLiteral("%1 · %2 × %3 · %4")
                               .arg(format)
                               .arg(_image.width())
                               .arg(_image.height())
                               .arg(size));
}

///
/// \brief Applies the theme's caption colour to the description line.
///
void ImageViewDialog::applyHeaderColors()
{
    QPalette palette = ui->infoLabel->palette();
    palette.setColor(QPalette::WindowText, AppColors::subtitleText());
    ui->infoLabel->setPalette(palette);
}

///
/// \brief Returns the picture scaled to the current zoom factor.
/// \return Pixmap to show, at its own size when the factor is 1.
///
/// Magnifying keeps the pixels square: a smooth transform at 400% would blur exactly the
/// detail someone zooms in to look at.
///
QPixmap ImageViewDialog::scaledPixmap() const
{
    const qreal scale = currentScale();
    if (_pixmap.isNull() || qFuzzyCompare(scale, qreal(1.0)))
        return _pixmap;

    const QSize target = (QSizeF(_pixmap.size()) * scale).toSize().expandedTo(QSize(1, 1));
    return _pixmap.scaled(target, Qt::KeepAspectRatio,
                          scale < 1.0 ? Qt::SmoothTransformation : Qt::FastTransformation);
}

///
/// \brief Returns the file filter and default suffix the picture's format calls for.
/// \param suffix Receives the default file suffix.
/// \return Save-dialog filter string.
///
QString ImageViewDialog::saveFilter(QString *suffix) const
{
    const QString format = _format.isEmpty() ? QStringLiteral("bin") : _format.toLower();
    *suffix = format;
    return QStringLiteral("%1 (*.%2);;%3 (*)")
        .arg(format.toUpper(), format, tr("All Files"));
}

///
/// \brief Returns how far this window steps away from the viewers already open.
/// \return Offset to add to the position the dialog was centred at.
///
/// The step counts the viewers that are open under the same main window, so closing one
/// gives its place back rather than sending the next picture ever further off screen. The
/// cascade restarts after a few windows for the same reason.
///
QPoint ImageViewDialog::cascadeOffset() const
{
    const QWidget *root = parentWidget() ? parentWidget()->window() : nullptr;
    if (!root)
        return {};

    int open = 0;
    const auto viewers = root->findChildren<ImageViewDialog *>();
    for (const ImageViewDialog *viewer : viewers) {
        if (viewer != this && viewer->isVisible())
            ++open;
    }

    const int step = (open % CascadeLength) * CascadeStep;
    return { step, step };
}
