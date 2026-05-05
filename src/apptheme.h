#pragma once

#include <QPalette>

///
/// \brief The AppTheme class
///
class AppTheme final
{
public:
    static QPalette lightPalette();
    static QPalette darkPalette();
    static QPalette systemPalette();
};
