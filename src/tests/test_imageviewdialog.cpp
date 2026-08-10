// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_imageviewdialog.cpp
/// \brief Tests the encoded-picture viewer dialog.
///

#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QTest>
#include <QToolButton>

#include "application.h"
#include "dialogs/imageviewdialog.h"
#include "testimages.h"
#include "widgets/formatbadge.h"
#include "widgets/imagecanvas.h"

using TestImages::encodedPng;

class TestImageViewDialog : public QObject
{
    Q_OBJECT

private slots:
    void encodedPictureIsDecodedAndDescribed();
    void undecodablePictureExplainsItselfAndStaysSaveable();
    void fittingMeasuresTheViewportItEndsUpWith();
    void fittingScalesToTheViewportInBothDirections();
    void switchingFittingOffReturnsToNaturalSize();
    void zoomSliderSetsTheScaleAndTakesOverFromFitting();
    void zoomButtonsStepTheSlider();
};

///
/// \brief A picture the dialog can decode is shown, measured, badged, and named.
///
void TestImageViewDialog::encodedPictureIsDecodedAndDescribed()
{
    const QByteArray png = encodedPng(QSize(20, 10));
    QVERIFY(!png.isEmpty());

    ImageViewDialog dialog;
    dialog.setImageName(QStringLiteral("ImagePNG"));
    dialog.setImageData(png);

    QCOMPARE(dialog.image().size(), QSize(20, 10));

    auto *canvas = dialog.findChild<ImageCanvas *>(QStringLiteral("imageCanvas"));
    QVERIFY(canvas);
    QVERIFY(!canvas->pixmap().isNull());

    auto *badge = dialog.findChild<FormatBadge *>(QStringLiteral("formatBadge"));
    QVERIFY(badge);
    QCOMPARE(badge->text(), QStringLiteral("PNG"));

    auto *name = dialog.findChild<QLabel *>(QStringLiteral("nameLabel"));
    QVERIFY(name);
    QCOMPARE(name->text(), QStringLiteral("ImagePNG"));

    auto *info = dialog.findChild<QLabel *>(QStringLiteral("infoLabel"));
    QVERIFY(info);
    QVERIFY2(info->text().startsWith(QStringLiteral("PNG · 20 × 10 · ")), qPrintable(info->text()));

    QVERIFY(dialog.findChild<QPushButton *>(QStringLiteral("copyButton"))->isEnabled());
    QVERIFY(dialog.findChild<QPushButton *>(QStringLiteral("saveButton"))->isEnabled());
}

///
/// \brief Bytes that decode to nothing say so, and can still be written to a file.
///
void TestImageViewDialog::undecodablePictureExplainsItselfAndStaysSaveable()
{
    ImageViewDialog dialog;
    dialog.setImageData(QByteArrayLiteral("not a picture at all"));

    QVERIFY(dialog.image().isNull());

    auto *canvas = dialog.findChild<ImageCanvas *>(QStringLiteral("imageCanvas"));
    QVERIFY(canvas);
    QVERIFY(canvas->pixmap().isNull());
    QVERIFY(!canvas->placeholderText().isEmpty());

    // An unreadable format leaves the badge with nothing to abbreviate.
    auto *badge = dialog.findChild<FormatBadge *>(QStringLiteral("formatBadge"));
    QVERIFY(badge);
    QCOMPARE(badge->text(), QStringLiteral("?"));

    // Copying an image nobody decoded is meaningless; saving the bytes is not.
    QVERIFY(!dialog.findChild<QPushButton *>(QStringLiteral("copyButton"))->isEnabled());
    QVERIFY(dialog.findChild<QPushButton *>(QStringLiteral("saveButton"))->isEnabled());
}

///
/// \brief A picture set before the dialog is laid out is fitted to the viewport it gets.
///
/// The viewport has no useful size until the first layout, and the dialog is never resized
/// on the way there, so a fit measured in the constructor would be the one left on screen.
///
void TestImageViewDialog::fittingMeasuresTheViewportItEndsUpWith()
{
    ImageViewDialog dialog;
    dialog.setImageData(encodedPng(QSize(100, 100)));
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    auto *canvas = dialog.findChild<ImageCanvas *>(QStringLiteral("imageCanvas"));
    auto *scrollArea = dialog.findChild<QScrollArea *>(QStringLiteral("scrollArea"));
    QVERIFY(canvas);
    QVERIFY(scrollArea);

    // The dialog opens far larger than the picture, so fitting fills the viewport with it.
    QTRY_VERIFY(canvas->pixmap().height() > 100);
    QVERIFY(canvas->pixmap().height() <= scrollArea->viewport()->height());
}

///
/// \brief Fitting scales a picture to the viewport, magnifying as readily as it shrinks.
///
void TestImageViewDialog::fittingScalesToTheViewportInBothDirections()
{
    ImageViewDialog dialog;
    dialog.resize(500, 400);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    auto *canvas = dialog.findChild<ImageCanvas *>(QStringLiteral("imageCanvas"));
    auto *fit = dialog.findChild<QPushButton *>(QStringLiteral("fitButton"));
    auto *scrollArea = dialog.findChild<QScrollArea *>(QStringLiteral("scrollArea"));
    auto *zoomLabel = dialog.findChild<QLabel *>(QStringLiteral("zoomLabel"));
    QVERIFY(canvas);
    QVERIFY(fit);
    QVERIFY(scrollArea);
    QVERIFY(zoomLabel);
    QVERIFY(fit->isChecked());

    const QSize huge(2000, 1000);
    dialog.setImageData(encodedPng(huge));
    QCOMPARE(dialog.image().size(), huge);
    QSize fitted = canvas->pixmap().size();
    QVERIFY2(fitted.width() < huge.width(), qPrintable(QString::number(fitted.width())));
    QVERIFY(fitted.width() <= scrollArea->viewport()->width());
    QVERIFY(fitted.height() <= scrollArea->viewport()->height());

    // The readout follows the fitted factor rather than staying at 100%.
    QVERIFY2(zoomLabel->text() != QStringLiteral("100%"), qPrintable(zoomLabel->text()));

    // A picture smaller than the viewport is magnified up to it, not left as a stamp.
    dialog.setImageData(encodedPng(QSize(8, 6)));
    fitted = canvas->pixmap().size();
    QVERIFY2(fitted.width() > 8, qPrintable(QString::number(fitted.width())));
    QVERIFY(fitted.width() <= scrollArea->viewport()->width());
    QVERIFY(fitted.height() <= scrollArea->viewport()->height());

    // Neither direction ever distorts the picture.
    QCOMPARE(qRound(qreal(fitted.width()) / fitted.height() * 100),
             qRound(8.0 / 6.0 * 100));
}

///
/// \brief Clicking the fit button off shows the picture at its own size.
///
void TestImageViewDialog::switchingFittingOffReturnsToNaturalSize()
{
    ImageViewDialog dialog;
    dialog.resize(500, 400);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    dialog.setImageData(encodedPng(QSize(100, 100)));

    auto *canvas = dialog.findChild<ImageCanvas *>(QStringLiteral("imageCanvas"));
    auto *fit = dialog.findChild<QPushButton *>(QStringLiteral("fitButton"));
    auto *slider = dialog.findChild<QSlider *>(QStringLiteral("zoomSlider"));
    auto *zoomLabel = dialog.findChild<QLabel *>(QStringLiteral("zoomLabel"));
    QVERIFY(canvas);
    QVERIFY(fit);
    QVERIFY(slider);
    QVERIFY(zoomLabel);
    QVERIFY(fit->isChecked());
    QVERIFY(canvas->pixmap().height() > 100);

    fit->click();
    QVERIFY(!fit->isChecked());
    QCOMPARE(slider->value(), 100);
    QCOMPARE(canvas->pixmap().size(), QSize(100, 100));
    QCOMPARE(zoomLabel->text(), QStringLiteral("100%"));

    // Fitting again picks the viewport back up.
    fit->click();
    QVERIFY(fit->isChecked());
    QVERIFY(canvas->pixmap().height() > 100);
}

///
/// \brief Moving the zoom slider scales the picture and stops the view from re-fitting it.
///
void TestImageViewDialog::zoomSliderSetsTheScaleAndTakesOverFromFitting()
{
    ImageViewDialog dialog;
    dialog.resize(500, 400);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    dialog.setImageData(encodedPng(QSize(20, 10)));

    auto *canvas = dialog.findChild<ImageCanvas *>(QStringLiteral("imageCanvas"));
    auto *fit = dialog.findChild<QPushButton *>(QStringLiteral("fitButton"));
    auto *slider = dialog.findChild<QSlider *>(QStringLiteral("zoomSlider"));
    auto *zoomLabel = dialog.findChild<QLabel *>(QStringLiteral("zoomLabel"));
    QVERIFY(canvas);
    QVERIFY(fit);
    QVERIFY(slider);
    QVERIFY(zoomLabel);
    QVERIFY(fit->isChecked());

    slider->setValue(200);
    QVERIFY(!fit->isChecked());
    QCOMPARE(canvas->pixmap().size(), QSize(40, 20));
    QCOMPARE(zoomLabel->text(), QStringLiteral("200%"));

    slider->setValue(50);
    QCOMPARE(canvas->pixmap().size(), QSize(10, 5));
    QCOMPARE(zoomLabel->text(), QStringLiteral("50%"));

    // Nothing to zoom means nothing to offer.
    dialog.setImageData(QByteArrayLiteral("not a picture at all"));
    QVERIFY(!slider->isEnabled());
    QVERIFY(zoomLabel->text().isEmpty());
}

///
/// \brief The minus and plus buttons move the zoom one slider page at a time.
///
void TestImageViewDialog::zoomButtonsStepTheSlider()
{
    ImageViewDialog dialog;
    dialog.resize(500, 400);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    dialog.setImageData(encodedPng(QSize(2000, 1000)));

    auto *fit = dialog.findChild<QPushButton *>(QStringLiteral("fitButton"));
    auto *slider = dialog.findChild<QSlider *>(QStringLiteral("zoomSlider"));
    auto *zoomIn = dialog.findChild<QToolButton *>(QStringLiteral("zoomInButton"));
    auto *zoomOut = dialog.findChild<QToolButton *>(QStringLiteral("zoomOutButton"));
    QVERIFY(fit);
    QVERIFY(slider);
    QVERIFY(zoomIn);
    QVERIFY(zoomOut);
    QVERIFY(fit->isChecked());

    // Stepping out of fitting carries on from the fitted factor; only the button itself
    // falls back to the natural size.
    const int fitted = slider->value();
    zoomIn->click();
    QVERIFY(!fit->isChecked());
    QCOMPARE(slider->value(), fitted + slider->pageStep());

    slider->setValue(200);
    zoomIn->click();
    QCOMPARE(slider->value(), 200 + slider->pageStep());

    zoomOut->click();
    zoomOut->click();
    QCOMPARE(slider->value(), 200 - slider->pageStep());
}

///
/// \brief Runs the suite under Application so the themed styles are in place.
/// \param argc Argument count.
/// \param argv Argument vector.
/// \return Test exit code.
///
int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestImageViewDialog test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_imageviewdialog.moc"
