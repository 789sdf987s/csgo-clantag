#pragma once

#include "utils\cx_fnv1.h"
#include "utils\cx_pcg32.h"
#include "utils\cx_strenc.h"
#include "utils\environment.h"

typedef void* (*InstantiateInterfaceFn)();
class InterfaceReg
{
public:
	InstantiateInterfaceFn	     m_CreateFn;
	const char* m_pName;

	InterfaceReg* m_pNext;
};

namespace utils
{
	const win::LDR_DATA_TABLE_ENTRY_T* GetModuleEntry(hashed_string moduleName);
	unsigned long                      ModulePatternScan(const win::LDR_DATA_TABLE_ENTRY_T* entry, const char* signature);
	void*                              GetInterface(unsigned long ppInterfaceList, hashed_string interfaceName, size_t interfaceNameLength);
}
