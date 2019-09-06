#include "winapi.h"
#include "utils.h"

void* _getImportFuncSafe(hashed_string importFuncName)
{
	auto       current_entry = env::ldr_data_entry();
	const auto first_entry   = current_entry;

	do
	{
		const env::exports_directory exports(current_entry->DllBase);

		if (exports)
		{
			auto export_index = exports.size();
			while (export_index--)
			{
				if (hash_compare(importFuncName, exports.name(export_index)))
					return (void*)(exports.address(export_index));
			}
		}

		current_entry = current_entry->load_order_next();

	} while (current_entry != first_entry);

	return nullptr;
}

#define DEFINE_IMPORT_FN(import_name) \
	decltype(import_name)* _##import_name = (decltype(import_name)*) _getImportFuncSafe((hash(#import_name)));

DEFINE_IMPORT_FN(CreateThread);
DEFINE_IMPORT_FN(DisableThreadLibraryCalls);
DEFINE_IMPORT_FN(FreeLibraryAndExitThread);
DEFINE_IMPORT_FN(ExitThread);

DEFINE_IMPORT_FN(GetModuleFileNameA);
DEFINE_IMPORT_FN(GetModuleHandleA);
DEFINE_IMPORT_FN(FindWindowA);

DEFINE_IMPORT_FN(GetTickCount);
DEFINE_IMPORT_FN(Sleep);