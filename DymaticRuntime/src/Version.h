#pragma once

// Client modified values
#define DY_APPLICATION_NAME "Dymatic Runtime"
#define DY_APPLICATION_EXECUTABLE "DymaticRuntime.exe"
#define DY_APPLICATION_VERSION_MAJOR 1
#define DY_APPLICATION_VERSION_MINOR 0
#define DY_APPLICATION_VERSION_PATCH 0
#define DY_APPLICATION_VERSION_BUILD 0
#define DY_APPLICATION_VERSION_SUFFIX ""
#define DY_APPLICATION_COPYRIGHT ""
#define DY_APPLICATION_TRADEMARK ""

// Built in values
#define DY_APPLICATION_VERSION_STRING \
    DY_TO_STRING(DY_APPLICATION_VERSION_MAJOR) "." \
    DY_TO_STRING(DY_APPLICATION_VERSION_MINOR) "." \
    DY_TO_STRING(DY_APPLICATION_VERSION_PATCH) "." \
    DY_TO_STRING(DY_APPLICATION_VERSION_BUILD)

#define DY_APPLICATION_VERSION DY_APPLICATION_VERSION_STRING " " DY_APPLICATION_VERSION_SUFFIX
