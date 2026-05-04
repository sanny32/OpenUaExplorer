#pragma once

#include <QIcon>
#include <QString>
#include <QWidget>

namespace Ui {
class DataAccessWidget;
}

class DataAccessWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataAccessWidget(QWidget *parent = nullptr);
    ~DataAccessWidget() override;

private:
    void populateDataTable();
    void configureToolbar();
    QIcon themedIcon(const QString &name) const;

    Ui::DataAccessWidget *ui;
};
