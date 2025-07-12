#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <basetsd.h>
#include <WTypesbase.h>
#include <intrin.h>
#include <string>
#include "Callstack-Spoofer.h"
#include "module_spoofing.h"

inline uintptr_t baseAddress;
inline uintptr_t getcr3;

