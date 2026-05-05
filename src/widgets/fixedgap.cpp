#include <QSizePolicy>
#include "fixedgap.h"

///
/// \brief FixedGap::FixedGap
/// \param width
/// \param parent
///
FixedGap::FixedGap(int width, QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(width);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
}
