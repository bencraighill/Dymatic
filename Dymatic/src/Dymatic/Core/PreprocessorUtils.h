#pragma once

#define DY_EXPAND_MACRO(x) x

#define DY_TO_STRING(x) DY_STRINGIFY_MACRO(x)
#define DY_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)
