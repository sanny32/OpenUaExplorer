// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesswidget_support.h
/// \brief Shared helpers for DataAccessWidget tests.
///

#pragma once

#include <QAbstractItemDelegate>
#include <QCoreApplication>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QFont>
#include <QImage>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QProxyStyle>
#include <QPushButton>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QStyleOptionViewItem>
#include <QStyleFactory>
#include <QTranslator>
#include <QTreeView>
#include <QTest>
#include <QTimer>

#include "appcolors.h"
#include "models/addressspacemimedata.h"
#include "models/dataaccessmodel.h"
#include "widgets/dataaccesswidget.h"
#include "widgets/dialogbuttonbox.h"

namespace {

class ItemPaletteStyle : public QProxyStyle
{
public:
    ///
    /// \brief Constructs a recording proxy over the Fusion style.
    ///
    ItemPaletteStyle()
        : QProxyStyle(QStyleFactory::create(QStringLiteral("fusion")))
    {
    }

    ///
    /// \brief Records the palette received for an item-view cell.
    /// \param element Control element being painted.
    /// \param option Style option carrying the cell palette.
    /// \param painter Painter receiving the rendering.
    /// \param widget Widget the control belongs to.
    ///
    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override
    {
        if (element == CE_ItemViewItem) {
            if (const auto *item = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
                text = item->palette.color(QPalette::Text);
                highlightedText = item->palette.color(QPalette::HighlightedText);
            }
        }
        QProxyStyle::drawControl(element, option, painter, widget);
    }

    mutable QColor text;
    mutable QColor highlightedText;
};

///
/// \brief Builds a node for drag/drop tests.
/// \param nodeClass OPC UA NodeClass value.
/// \return Node info item.
///
OpcUaNodeInfo makeDroppedNode(int nodeClass)
{
    OpcUaNodeInfo node;
    node.nodeId = nodeClass == OpcUa::Variable
        ? QStringLiteral("ns=2;s=Temperature")
        : QStringLiteral("ns=2;s=Device");
    node.browseName = node.nodeId;
    node.displayName = nodeClass == OpcUa::Variable
        ? QStringLiteral("Temperature")
        : QStringLiteral("Device");
    node.nodeClass = nodeClass;
    node.hasChildren = nodeClass != OpcUa::Variable;
    return node;
}

///
/// \brief Sends drag-enter and drop events to the data table viewport.
/// \param view Target table view.
/// \param mimeData Drag MIME data.
/// \return Whether the drag-enter event was accepted.
///
bool dropOnDataView(QTreeView *view, const QMimeData *mimeData)
{
    QDragEnterEvent enterEvent(QPoint(4, 4), Qt::CopyAction, mimeData,
                               Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(view->viewport(), &enterEvent);

    QDropEvent dropEvent(QPointF(4, 4), Qt::CopyAction, mimeData,
                         Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(view->viewport(), &dropEvent);
    return enterEvent.isAccepted();
}

///
/// \brief Selects every monitored node of the data view.
/// \param view Data view to select in.
///
/// QTreeView::selectAll() only reaches the rows it has laid out, which an unshown view has
/// none of, so the selection is made through the selection model instead.
///
void selectAllRows(QTreeView *view)
{
    QAbstractItemModel *model = view->model();
    if (model->rowCount() == 0) {
        view->selectionModel()->clearSelection();
        return;
    }
    const QItemSelection selection(model->index(0, 0),
                                   model->index(model->rowCount() - 1,
                                                model->columnCount() - 1));
    view->selectionModel()->select(selection,
                                   QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

///
/// \brief Builds node details for Data Access tests.
/// \return Node details item.
///
OpcUaNodeDetails makeNodeDetails()
{
    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=Temperature");
    details.displayName = QStringLiteral("Temperature");
    details.nodeClass = OpcUa::Variable;
    details.value = 21.5;
    details.dataTypeId = QStringLiteral("ns=0;i=11");
    details.status = QStringLiteral("Good");
    return details;
}

///
/// \brief Builds Boolean node details for the double-click toggle tests.
/// \param value Current value of the node.
/// \param writable Whether the UserAccessLevel grants CurrentWrite.
/// \return Node details item.
///
OpcUaNodeDetails makeBooleanNodeDetails(bool value, bool writable)
{
    OpcUaNodeDetails details = makeNodeDetails();
    details.nodeId = QStringLiteral("ns=2;s=Locked");
    details.displayName = QStringLiteral("Locked");
    details.value = value;
    details.valueType = 0;
    details.dataTypeId = QStringLiteral("ns=0;i=1");
    details.userAccessLevel = writable
        ? (OpcUa::CurrentRead | OpcUa::CurrentWrite)
        : OpcUa::CurrentRead;
    return details;
}

///
/// \brief Double-clicks the centre of a cell in the data table.
/// \param view Data table view.
/// \param row Row to click.
/// \param column Column to click.
///
/// The press and the double click are sent straight to the viewport: the view only
/// emits doubleClicked() when the double click lands on the cell it recorded on the
/// preceding press, and the offscreen platform does not synthesise that pair.
///
void doubleClickCell(QTreeView *view, int row, int column)
{
    const QModelIndex index = view->model()->index(row, column);
    QVERIFY(index.isValid());
    view->scrollTo(index);

    const QPoint pos = view->visualRect(index).center();
    const QPointF global = view->viewport()->mapToGlobal(pos);
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(pos), global,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent doubleClick(QEvent::MouseButtonDblClick, QPointF(pos), global,
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(view->viewport(), &press);
    QCoreApplication::sendEvent(view->viewport(), &doubleClick);
}

///
/// \brief Builds a variable node with a distinct NodeId.
/// \param index Index folded into the NodeId and display name.
/// \return Node info item.
///
OpcUaNodeInfo makeVariable(int index)
{
    OpcUaNodeInfo node;
    node.nodeId = QStringLiteral("ns=2;s=Var%1").arg(index);
    node.displayName = QStringLiteral("Var%1").arg(index);
    node.nodeClass = OpcUa::Variable;
    node.hasChildren = false;
    return node;
}

///
/// \brief Answers the next modal dialog, waiting for it to appear.
/// \param answer Standard button to click once the dialog is up.
///
void answerNextDialog(DialogButtonBox::StandardButton answer)
{
    QTimer::singleShot(0, qApp, [answer]() {
        auto *modal = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!modal) {
            answerNextDialog(answer);
            return;
        }
        auto *buttons = modal->findChild<DialogButtonBox *>();
        QVERIFY(buttons);
        QPushButton *button = buttons->button(answer);
        QVERIFY(button);
        QTest::mouseClick(button, Qt::LeftButton);
    });
}

///
/// \brief Dismisses a modal dialog if one appears, recording that it did.
/// \param seen Set to true when a dialog had to be dismissed.
///
/// The caller must drain the event queue afterwards so the pending check runs
/// exactly once, while \a seen is still alive.
///
void watchForDialog(bool *seen)
{
    QTimer::singleShot(0, qApp, [seen]() {
        auto *modal = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!modal)
            return;
        *seen = true;
        modal->reject();
    });
}

///
/// \brief Paints the table's viewport into an image the test can inspect.
/// \param view View to render.
/// \return Rendered viewport on a black background.
///
QImage renderViewport(QTreeView *view)
{
    QImage image(view->viewport()->size(), QImage::Format_ARGB32);
    image.fill(Qt::black);
    view->viewport()->render(&image);
    return image;
}

} // namespace