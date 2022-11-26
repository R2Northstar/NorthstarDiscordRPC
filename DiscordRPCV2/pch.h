#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include "spdlog/spdlog.h"

#include "lib/plugin_abi.h"

#define EXPORT extern "C"  __declspec(dllexport)

#endif
