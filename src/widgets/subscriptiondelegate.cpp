#include <QComboBox>

#include "subscriptiondelegate.h"

///
/// \brief SubscriptionDelegate::SubscriptionDelegate
/// \param subscriptionNames
/// \param parent
///
SubscriptionDelegate::SubscriptionDelegate(QStringList subscriptionNames, QObject *parent)
    : QStyledItemDelegate(parent)
    , _subscriptionNames(std::move(subscriptionNames))
{
}

///
/// \brief SubscriptionDelegate::createEditor
/// \param parent
/// \param option
/// \param index
/// \return
///
QWidget *SubscriptionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &,
                                            const QModelIndex &) const
{
    QComboBox *combo = new QComboBox(parent);
    combo->addItem(QStringLiteral("—"), QString());
    for (const QString &name : _subscriptionNames)
        combo->addItem(name, name);
    return combo;
}

///
/// \brief SubscriptionDelegate::setEditorData
/// \param editor
/// \param index
///
void SubscriptionDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *combo = static_cast<QComboBox *>(editor);
    const QString current = index.data(Qt::EditRole).toString();
    const int i = combo->findData(current);
    combo->setCurrentIndex(i >= 0 ? i : 0);
}

///
/// \brief SubscriptionDelegate::setModelData
/// \param editor
/// \param model
/// \param index
///
void SubscriptionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                        const QModelIndex &index) const
{
    QComboBox *combo = static_cast<QComboBox *>(editor);
    model->setData(index, combo->currentData(), Qt::EditRole);
}
