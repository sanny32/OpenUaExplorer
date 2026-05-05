#pragma once

#include <QIcon>
#include <QString>
#include <QWidget>

namespace Ui {
class DataAccessWidget;
}

class DataAccessModel;

class DataAccessWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataAccessWidget(QWidget *parent = nullptr);
    ~DataAccessWidget() override;

private:
    void setupDataView();
    void configureToolbar();
    QIcon themedIcon(const QString &name) const;

    Ui::DataAccessWidget *ui;
    DataAccessModel *_model;
};
