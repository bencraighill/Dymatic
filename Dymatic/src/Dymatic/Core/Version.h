#pragma once

#include "Dymatic/Core/PreprocessorUtils.h"

#define DY_VERSION_MAJOR 23
#define DY_VERSION_MINOR 1
#define DY_VERSION_PATCH 1

#define DY_VERSION_SUFFIX "Development"

#define DY_VERSION_STRING \
    DY_TO_STRING(DY_VERSION_MAJOR) "." \
    DY_TO_STRING(DY_VERSION_MINOR) "." \
    DY_TO_STRING(DY_VERSION_PATCH)

#define DY_VERSION DY_VERSION_STRING " (" DY_VERSION_SUFFIX ")"

#define DY_VERSION_COPYRIGHT "© 2023 Dymatic Technologies"
#define DY_VERSION_COPYRIGHT_SAFE "(c) 2023 Dymatic Technologies"
#define DY_VERSION_TRADEMARK "Dymatic Engine®, All Rights Reserved"
