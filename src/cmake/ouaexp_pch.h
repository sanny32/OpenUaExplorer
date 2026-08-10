#ifndef OUAEXP_PCH_H
#define OUAEXP_PCH_H

// Precompiled header for the ouaexp libraries. It gathers the heavy, stable
// headers pulled in by nearly every translation unit so MSVC/GCC/Clang parse
// them once per target instead of once per source file. Keep this list to
// widely used, rarely changing headers only; project headers must not appear
// here or every edit would invalidate the PCH.
//
// Qt Widgets must stay out: ouaexp_core builds against Qt Gui only, and a
// forced include here would put the module back on its compile line. Widget
// headers belong in ouaexp_widgets_pch.h.

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QHash>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <QColor>
#include <QIcon>

#endif // OUAEXP_PCH_H
