// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_appstyle.cpp
/// \brief Tests how the application style paints item-view cells.
///

#include <QBrush>
#include <QColor>
#include <QCoreApplication>
#include <QHeaderView>
#include <QImage>
#include <QItemSelectionModel>
#include <QPainter>
#include <QPalette>
#include <QSet>
#include <QStandardItemModel>
#include <QStyleFactory>
#include <QStyleOptionFocusRect>
#include <QTableView>
#include <QTest>
#include <QTreeView>
#include <QWidget>

#include "application.h"
#include "style/appstyle.h"
#include "style/macappstyle.h"
#include "style/qlementineappstyle.h"
#include "widgets/themedtoolbutton.h"

///
/// \brief Tests for QlementineAppStyle's item painting.
///
class TestAppStyle : public QObject
{
    Q_OBJECT

private slots:
    void centeredCellsKeepTheModelForeground();
    void autoRaiseToolButtonsHaveNoMacBezel();
    void stripedViewsKeepTheSystemSelectionColor();
    void unstripedViewsUseTheSystemSelectionColor();
    void nativeUnstripedViewsIgnoreTheSystemSelectionColor();
    void hoverUsesTheNativeFillOverModelBackground();
    void itemViewFocusFrameIsHidden();
    void focusedSelectionHasNoCellFrame();
};

namespace {

/// \brief Unmistakable stand-in for the OS accent, so the check does not depend on it.
const QColor loudHighlight(0xff, 0x00, 0xff);
const QColor loudModelBackground(0xff, 0x00, 0x00);

///
/// \brief Renders a two-row tree with its first row selected.
/// \param viaAppStyle Whether the proxy style paints, or the bare Windows 11 style.
/// \param striped Whether the view uses alternating row colours.
/// \param focused Whether the selected item owns keyboard focus.
/// \return Rendered viewport.
///
QImage renderSelection(bool viaAppStyle, bool striped, bool focused = false)
{
    QStandardItemModel model(2, 1);
    model.setItem(0, 0, new QStandardItem(QStringLiteral("First")));
    model.setItem(1, 0, new QStandardItem(QStringLiteral("Second")));

    // Naming the base style keeps the case under test off the platform's default one.
    const QScopedPointer<QStyle> style(
        viaAppStyle ? static_cast<QStyle *>(new AppStyle(QStringLiteral("windows11")))
                    : QStyleFactory::create(QStringLiteral("windows11")));
    if (style.isNull())
        return {};

    QTreeView view;
    view.setStyle(style.data());
    view.setAllColumnsShowFocus(true);
    QPalette palette = view.palette();
    palette.setColor(QPalette::Highlight, loudHighlight);
    view.setPalette(palette);
    view.setAlternatingRowColors(striped);
    view.setModel(&model);
    view.setHeaderHidden(true);
    view.resize(160, 60);
    view.show();
    if (!QTest::qWaitForWindowExposed(&view))
        return {};
    const QModelIndex selected = model.index(0, 0);
    view.setCurrentIndex(selected);
    view.selectionModel()->select(selected, QItemSelectionModel::Select);
    if (focused)
        view.setFocus(Qt::OtherFocusReason);
    else
        view.clearFocus();
    QCoreApplication::processEvents();

    const QImage rendered = view.viewport()->grab().toImage();
    view.hide();
    // The style outlives the view only if the view stops using it first.
    view.setStyle(nullptr);
    return rendered;
}

///
/// \brief Renders a two-row tree while the first row is hovered.
/// \param modelBackground Optional background supplied by the model for the hovered item.
/// \return Rendered viewport.
///
QImage renderHover(const QColor &modelBackground)
{
    QStandardItemModel model(2, 1);
    auto *first = new QStandardItem(QStringLiteral("First"));
    if (modelBackground.isValid())
        first->setBackground(modelBackground);
    model.setItem(0, 0, first);
    model.setItem(1, 0, new QStandardItem(QStringLiteral("Second")));

    AppStyle style(QStringLiteral("windows11"));
    QTreeView view;
    view.setStyle(&style);
    view.setAlternatingRowColors(true);
    QPalette palette = view.palette();
    palette.setColor(QPalette::Highlight, loudHighlight);
    view.setPalette(palette);
    view.setModel(&model);
    view.setHeaderHidden(true);
    view.resize(160, 60);
    view.show();
    if (!QTest::qWaitForWindowExposed(&view))
        return {};

    QTest::mouseMove(view.viewport(), view.visualRect(model.index(0, 0)).center());
    QCoreApplication::processEvents();
    const QImage rendered = view.viewport()->grab().toImage();
    view.hide();
    view.setStyle(nullptr);
    return rendered;
}

///
/// \brief Renders an item-view focus primitive through the proxy or bare Fusion style.
/// \param viaAppStyle Whether AppStyle or the bare style paints the primitive.
/// \return Rendered focus rectangle canvas.
///
QImage renderItemViewFocus(bool viaAppStyle)
{
    const QScopedPointer<QStyle> style(
        viaAppStyle ? static_cast<QStyle *>(new AppStyle(QStringLiteral("fusion")))
                    : QStyleFactory::create(QStringLiteral("fusion")));
    if (style.isNull())
        return {};

    QTreeView view;
    view.setStyle(style.data());
    QImage image(80, 28, QImage::Format_ARGB32_Premultiplied);
    image.fill(loudModelBackground);

    QStyleOptionFocusRect option;
    option.initFrom(&view);
    option.rect = image.rect().adjusted(2, 2, -2, -2);
    option.state |= QStyle::State_HasFocus | QStyle::State_KeyboardFocusChange;

    QPainter painter(&image);
    style->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &painter, &view);
    painter.end();
    view.setStyle(nullptr);
    return image;
}

///
/// \brief Reports whether a rendering contains at least one pixel of an exact colour.
/// \param image Rendered viewport.
/// \param color Colour to look for.
/// \return True when the colour occurs.
///
bool containsColor(const QImage &image, const QColor &color)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y) == color)
                return true;
        }
    }
    return false;
}

///
/// \brief Reports whether the first row uses a colour across the viewport width.
/// \param image Rendered viewport.
/// \param color Expected row colour.
/// \return True when both edges of the first row use the colour.
///
bool firstRowUsesColor(const QImage &image, const QColor &color)
{
    const int y = qMin(7, image.height() - 1);
    return image.pixelColor(1, y) == color
        && image.pixelColor(image.width() - 2, y) == color;
}

///
/// \brief Counts pixels of an exact colour in a rendering.
/// \param image Rendered viewport.
/// \param color Colour to count.
/// \return Number of matching pixels.
///
int colorPixelCount(const QImage &image, const QColor &color)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y) == color)
                ++count;
        }
    }
    return count;
}

} // namespace

#if defined(HAVE_QLEMENTINE_APP_STYLE)

namespace {

///
/// \brief Renders a single-cell table styled by the application style.
/// \param alignment Text alignment of the cell.
/// \param textColor Foreground the model contributes, or an invalid colour for none.
/// \return Rendered viewport.
///
QImage renderCell(Qt::Alignment alignment, const QColor &textColor)
{
    QStandardItemModel model(1, 1);
    auto *item = new QStandardItem(QStringLiteral("Good"));
    item->setTextAlignment(alignment);
    if (textColor.isValid())
        item->setForeground(QBrush(textColor));
    model.setItem(0, 0, item);

    QlementineAppStyle style;
    QTableView view;
    view.setStyle(&style);
    view.setModel(&model);
    view.horizontalHeader()->hide();
    view.verticalHeader()->hide();
    view.resize(160, 60);
    view.show();
    if (!QTest::qWaitForWindowExposed(&view))
        return {};

    const QImage rendered = view.viewport()->grab().toImage();
    view.hide();
    // The style outlives the view only if the view stops using it first.
    view.setStyle(nullptr);
    return rendered;
}

///
/// \brief Reports whether a rendering contains a clearly green pixel.
/// \param image Rendered viewport.
/// \return True when a pixel is dominated by its green channel.
///
/// Text is antialiased, so the drawn colour is matched by hue rather than exactly.
///
bool containsGreenText(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (pixel.green() > pixel.red() + 30 && pixel.green() > pixel.blue() + 30)
                return true;
        }
    }
    return false;
}

///
/// \brief Renders a tool button of the macOS style on a plain white backdrop.
/// \param autoRaise Whether the button is flat until hovered.
/// \return Rendered button.
///
QImage renderMacToolButton(bool autoRaise)
{
    MacAppStyle style;
    QWidget host;
    host.setAutoFillBackground(true);
    QPalette palette = host.palette();
    palette.setColor(QPalette::Window, Qt::white);
    host.setPalette(palette);
    host.resize(80, 60);

    ThemedToolButton button(&host);
    button.setStyle(&style);
    button.setAutoRaise(autoRaise);
    button.setGeometry(20, 15, 28, 24);

    host.show();
    if (!QTest::qWaitForWindowExposed(&host))
        return {};

    const QImage rendered = host.grab(button.geometry()).toImage();
    host.hide();
    // The style outlives the button only if the button stops using it first.
    button.setStyle(nullptr);
    return rendered;
}

///
/// \brief Counts the distinct colours of a rendering.
/// \param image Rendered widget.
/// \return Number of distinct pixel colours.
///
int distinctColorCount(const QImage &image)
{
    QSet<QRgb> colors;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x)
            colors.insert(image.pixel(x, y));
    }
    return colors.size();
}

} // namespace

///
/// \brief Centred columns honour a model's Qt::ForegroundRole like left-aligned ones do.
///
/// The style draws centred item text itself, which used to drop the colour the delegate
/// had put into the item palette, leaving greyed-out or status-coloured cells unstyled.
///
void TestAppStyle::centeredCellsKeepTheModelForeground()
{
    const QColor modelForeground(0, 150, 64);

    const QImage centered = renderCell(Qt::AlignCenter, modelForeground);
    QVERIFY(!centered.isNull());
    QVERIFY(containsGreenText(centered));

    const QImage leftAligned = renderCell(Qt::AlignLeft | Qt::AlignVCenter, modelForeground);
    QVERIFY(!leftAligned.isNull());
    QVERIFY(containsGreenText(leftAligned));

    // Without a model foreground the theme colour still decides.
    const QImage themed = renderCell(Qt::AlignCenter, QColor());
    QVERIFY(!themed.isNull());
    QVERIFY(!containsGreenText(themed));
}

///
/// \brief Flat tool buttons stay borderless under the macOS style.
///
/// The style bezels ThemedToolButtons so toolbar rows line up, which drew a frame around
/// the trend panel's collapse chevron as well.
///
void TestAppStyle::autoRaiseToolButtonsHaveNoMacBezel()
{
    const QImage bezeled = renderMacToolButton(false);
    QVERIFY(!bezeled.isNull());
    QVERIFY(distinctColorCount(bezeled) > 1);

    const QImage flat = renderMacToolButton(true);
    QVERIFY(!flat.isNull());
    QCOMPARE(distinctColorCount(flat), 1);
}

#else

void TestAppStyle::centeredCellsKeepTheModelForeground()
{
    QSKIP("Built without the Qlementine application style.");
}

void TestAppStyle::autoRaiseToolButtonsHaveNoMacBezel()
{
    QSKIP("Built without the Qlementine application style.");
}

#endif // HAVE_QLEMENTINE_APP_STYLE

///
/// \brief Striped item views preserve the selection colour supplied by the system palette.
///
void TestAppStyle::stripedViewsKeepTheSystemSelectionColor()
{
    if (!QStyleFactory::keys().contains(QStringLiteral("windows11"), Qt::CaseInsensitive))
        QSKIP("Only the native Windows 11 style ties the selection fill to row stripes.");

    const QImage proxied = renderSelection(true, true);
    QVERIFY(!proxied.isNull());
    QVERIFY(containsColor(proxied, loudHighlight));

    const QImage bare = renderSelection(false, true);
    QVERIFY(!bare.isNull());
    QVERIFY(containsColor(bare, loudHighlight));
}

///
/// \brief Unstriped item views use the selection colour supplied by the system palette.
///
void TestAppStyle::unstripedViewsUseTheSystemSelectionColor()
{
    if (!QStyleFactory::keys().contains(QStringLiteral("windows11"), Qt::CaseInsensitive))
        QSKIP("Only the native Windows 11 style replaces unstriped selection colours.");

    const QImage proxied = renderSelection(true, false);
    const QImage bare = renderSelection(false, false);
    QVERIFY(!proxied.isNull());
    QVERIFY(!bare.isNull());
    QVERIFY(colorPixelCount(proxied, loudHighlight)
            > colorPixelCount(bare, loudHighlight));
}

///
/// \brief The native control demonstrates why unstriped views need the proxy override.
///
void TestAppStyle::nativeUnstripedViewsIgnoreTheSystemSelectionColor()
{
    if (!QStyleFactory::keys().contains(QStringLiteral("windows11"), Qt::CaseInsensitive))
        QSKIP("Only the native Windows 11 style replaces unstriped selection colours.");

    const QImage bare = renderSelection(false, false);
    QVERIFY(!bare.isNull());
    QVERIFY(!firstRowUsesColor(bare, loudHighlight));
}

///
/// \brief Hover uses the native neutral fill even when the model supplies a background.
///
void TestAppStyle::hoverUsesTheNativeFillOverModelBackground()
{
    if (!QStyleFactory::keys().contains(QStringLiteral("windows11"), Qt::CaseInsensitive))
        QSKIP("Only the native Windows 11 style supplies this hover fill.");

    const QImage plain = renderHover(QColor());
    const QImage modelColoured = renderHover(loudModelBackground);
    QVERIFY(!plain.isNull());
    QVERIFY(!modelColoured.isNull());
    QCOMPARE(modelColoured, plain);
    QVERIFY(!containsColor(modelColoured, loudHighlight));
}

///
/// \brief Item views suppress the current-cell focus frame.
///
void TestAppStyle::itemViewFocusFrameIsHidden()
{
    const QImage proxied = renderItemViewFocus(true);
    const QImage bare = renderItemViewFocus(false);
    QVERIFY(!proxied.isNull());
    QVERIFY(!bare.isNull());

    const int pixelCount = proxied.width() * proxied.height();
    QCOMPARE(colorPixelCount(proxied, loudModelBackground), pixelCount);
    QVERIFY(colorPixelCount(bare, loudModelBackground) < pixelCount);
}

///
/// \brief Keyboard focus does not add a frame around the current selected cell.
///
void TestAppStyle::focusedSelectionHasNoCellFrame()
{
    if (!QStyleFactory::keys().contains(QStringLiteral("windows11"), Qt::CaseInsensitive))
        QSKIP("Only the native Windows 11 style draws this cell frame.");

    const QImage focused = renderSelection(true, true, true);
    const QImage unfocused = renderSelection(true, true, false);
    QVERIFY(!focused.isNull());
    QVERIFY(!unfocused.isNull());
    QCOMPARE(focused, unfocused);

    const QImage nativeFocused = renderSelection(false, true, true);
    const QImage nativeUnfocused = renderSelection(false, true, false);
    QVERIFY(!nativeFocused.isNull());
    QVERIFY(!nativeUnfocused.isNull());
    QVERIFY(nativeFocused != nativeUnfocused);
}
int main(int argc, char *argv[])
{
    // The style reads the application theme in its constructor.
    Application app(argc, argv);
    TestAppStyle test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_appstyle.moc"
