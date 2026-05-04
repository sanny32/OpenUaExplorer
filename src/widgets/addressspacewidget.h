#pragma once

#include <QWidget>

namespace Ui {
class AddressSpaceWidget;
}

class QTreeWidgetItem;

class AddressSpaceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AddressSpaceWidget(QWidget *parent = nullptr);
    ~AddressSpaceWidget() override;

private:
    void populateAddressTree();
    void populateNodeInfo();
    QTreeWidgetItem *addItem(QTreeWidgetItem *parent, const QString &text, const QString &iconName);
    QIcon themedIcon(const QString &name) const;

    Ui::AddressSpaceWidget *ui;
};
