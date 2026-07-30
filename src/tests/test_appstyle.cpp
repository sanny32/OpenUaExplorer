// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_appstyle.cpp
/// \brief Tests how the application style paints item-view cells.
///

#include <QBrush>
#include <QColor>
#include <QHeaderView>
#include <QImage>
#include <QStandardItemModel>
#include <QTableView>
#include <QTest>

#include "application.h"
#include "style/qlementineappstyle.h"

///
/// \brief Tests for QlementineAppStyle's item painting.
///
class TestAppStyle : public QObject
{
    Q_OBJECT

private slots:
    void centeredCellsKeepTheModelForeground();
};

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

#else

void TestAppStyle::centeredCellsKeepTheModelForeground()
{
    QSKIP("Built without the Qlementine application style.");
}

#endif // HAVE_QLEMENTINE_APP_STYLE

int main(int argc, char *argv[])
{
    // The style reads the application theme in its constructor.
    Application app(argc, argv);
    TestAppStyle test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_appstyle.moc"
