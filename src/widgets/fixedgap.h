#pragma once

#include <QWidget>

class FixedGap : public QWidget
{
    Q_OBJECT

public:
    explicit FixedGap(int width, QWidget *parent = nullptr);
};
