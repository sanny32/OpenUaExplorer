#pragma once

#include <QString>

///
/// \brief Describes one application log message.
///
struct LogItem
{
    enum class Level { Info, Warning, Error };

    QString timestamp;
    Level   level;
    QString source;
    QString message;
};
