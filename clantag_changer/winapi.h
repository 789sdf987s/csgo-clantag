#pragma once
#include <Windows.h>

#define DECLARE_IMPORT_FN(import_name) extern decltype(import_name)* _##import_name;

DECLARE_IMPORT_FN(CreateThread);
DECLARE_IMPORT_FN(DisableThreadLibraryCalls);
DECLARE_IMPORT_FN(FreeLibraryAndExitThread);
DECLARE_IMPORT_FN(ExitThread);

DECLARE_IMPORT_FN(GetModuleFileNameA);
DECLARE_IMPORT_FN(GetModuleHandleA);
DECLARE_IMPORT_FN(FindWindowA);

DECLARE_IMPORT_FN(GetTickCount);
DECLARE_IMPORT_FN(Sleep);
