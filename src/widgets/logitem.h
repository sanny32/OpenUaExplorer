#pragma once

#include <QString>

struct LogItem
{
    enum class Level { Info, Warning, Error };

    QString timestamp;
    Level   level;
    QString source;
    QString message;
};
