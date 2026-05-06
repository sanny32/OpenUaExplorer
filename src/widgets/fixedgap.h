#pragma once

#include <QWidget>

///
/// \brief Fixed-width spacer widget.
///
class FixedGap : public QWidget
{
    Q_OBJECT

public:
    explicit FixedGap(int width, QWidget *parent = nullptr);
};
