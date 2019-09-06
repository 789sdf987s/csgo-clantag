#include <stdio.h>
#include <Windows.h>
#include "utils.h"
#include "winapi.h"

// dumb++
#pragma warning (push)
#pragma warning (disable : 6011)

using SendClantagChangedFn = void(__fastcall*)(const char*, const char*);
using Engine_IsInGameFn    = bool(__fastcall*)(void*);

HMODULE g_dll    = nullptr;
void*   g_engine = nullptr;

__forceinline void ExitCheat()
{
	if (g_dll)
		FreeLibraryAndExitThread(g_dll, 0);
}

DWORD __stdcall MainThread(LPVOID lpThreadParameter)
{
	SendClantagChangedFn SendClantagChanged = nullptr;
	Engine_IsInGameFn    Engine_IsInGame = nullptr;

	long serverbrowser_wait_time = 1200; // 120 seconds

	char filename[MAX_PATH];
	_GetModuleFileNameA(NULL, filename, MAX_PATH);

	// make sure we're actually injected @ csgo
	if (std::string(filename).find(strenc("\\Steam\\steamapps\\common\\Counter-Strike Global Offensive\\csgo.exe")) == std::string::npos)
	{
		ExitCheat();
	}

	while (!_GetModuleHandleA(charenc("serverbrowser.dll")))
	{
		if (serverbrowser_wait_time < 1)
			ExitCheat();

		--serverbrowser_wait_time;
		_Sleep(100);
	}

	const win::LDR_DATA_TABLE_ENTRY_T* engineEntry = utils::GetModuleEntry(hash("engine.dll"));

	if (!engineEntry)
	{
		ExitCheat();
	}

	unsigned long dwInterfaceList = utils::ModulePatternScan(engineEntry, charenc("8B 35 ? ? ? ? 57 85 F6 74 38 8B 7D 08"));
	SendClantagChanged            = (SendClantagChangedFn)utils::ModulePatternScan(engineEntry, charenc("53 56 57 8B DA 8B F9 FF"));

	if (!dwInterfaceList || !SendClantagChanged)
	{
		ExitCheat();
	}

	g_engine = utils::GetInterface(dwInterfaceList + 2, hash("VEngineClient0"), 14);

	if(!g_engine)
	{
		ExitCheat();
	}
		
	Engine_IsInGame = (Engine_IsInGameFn)(*(unsigned long**)g_engine)[26];

	if (!Engine_IsInGame)
	{
		ExitCheat();
	}

	DWORD  next  = 0;
	size_t index = 0;

	constexpr size_t ct_count = 14;

	const char clantag[]         = { "me!me!me!" };
	char       buffer[ct_count + 1] = { 0 }; // max clantag length

	constexpr size_t length = sizeof(clantag) - 1;
	constexpr size_t count  = 14 + length;

	for (;;)
	{
		if (Engine_IsInGame(g_engine) && _GetTickCount() > next)
		{
			long amt = (ct_count - 1) - index;

			for (long i = 0; i != ct_count; ++i)
			{
				if (i > amt && i <= amt + length)
					buffer[i] = clantag[i - amt - 1];
				else
					buffer[i] = ' ';
			}

			SendClantagChanged(buffer, clantag);
			++index;

			if (index > count)
			{
				index = 0;
			}

			next = _GetTickCount() + 300;
		}

		_Sleep(10);
	}

	return NULL;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		if (hinstDLL)
		{
			_DisableThreadLibraryCalls(hinstDLL);
			g_dll = hinstDLL;

		}

		_CreateThread(NULL, 0, MainThread, nullptr, 0, nullptr);
	}

	return TRUE;
}

#pragma warning (pop)
