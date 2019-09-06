#include <stdio.h>
#include <Windows.h>
#include "utils.h"
#include "winapi.h"

// dumb++
#pragma warning (push)
#pragma warning (disable : 6011)

using SendClantagChangedFn = void(__fastcall*)(const char*, const char*);
using Engine_IsInGameFn    = bool(__fastcall*)(void*);

HMODULE g_dll         = nullptr;
void*   g_engine      = nullptr;
void**  g_localplayer = nullptr;

__forceinline void ExitCheat()
{
	if (g_dll)
		_FreeLibraryAndExitThread(g_dll, 0);
	else
		_ExitThread(0);
}

DWORD __stdcall MainThread(LPVOID lpThreadParameter)
{
	SendClantagChangedFn SendClantagChanged = nullptr;
	Engine_IsInGameFn    Engine_IsInGame = nullptr;

	long serverbrowser_wait_time = 1200; // 120 seconds

	char filename[MAX_PATH];
	_GetModuleFileNameA(NULL, filename, MAX_PATH);

	// make sure we're actually injected @ csgo
	if (std::string(filename).find(strenc("\\steamapps\\")) == std::string::npos
	 || std::string(filename).find(strenc("\\Counter-Strike Global Offensive\\")) == std::string::npos)
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

	_Sleep(100);

	const win::LDR_DATA_TABLE_ENTRY_T* clientEntry = utils::GetModuleEntry(hash("client_panorama.dll"));
	const win::LDR_DATA_TABLE_ENTRY_T* engineEntry = utils::GetModuleEntry(hash("engine.dll"));

	if (!clientEntry || !engineEntry)
	{
		ExitCheat();
	}

	DWORD dwLocalPlayer   = utils::ModulePatternScan(clientEntry, charenc("8B 0D ? ? ? ? 83 FF FF 74 07"));
	DWORD dwInterfaceList = utils::ModulePatternScan(engineEntry, charenc("8B 35 ? ? ? ? 57 85 F6 74 38 8B 7D 08"));

	SendClantagChanged = (SendClantagChangedFn)utils::ModulePatternScan(engineEntry, charenc("53 56 57 8B DA 8B F9 FF"));

	if (!dwLocalPlayer || !dwInterfaceList || !SendClantagChanged)
	{
		ExitCheat();
	}

	g_engine      = utils::GetInterface(dwInterfaceList + 2, hash("VEngineClient0"), 14);
	g_localplayer = *(void***)(dwLocalPlayer + 2);

	if(!g_engine || !g_localplayer)
	{
		ExitCheat();
	}
		
	Engine_IsInGame = (Engine_IsInGameFn)(*(unsigned long**)g_engine)[26];

	if (!Engine_IsInGame)
	{
		ExitCheat();
	}

	size_t pos = 0;
	long last = strlen(filename);

	for (long i = last; i >= 0; --i)
	{
		if (filename[i] == '\\')
		{
			pos = i;
			break;
		}
	}

	const char clantag[14] = { "me!me!me!" };

	if (pos)
	{
		std::string config = std::string(filename).substr(0, pos);
		config.assign(charenc("clantag.txt"));

		FILE* file = nullptr;
		fopen_s(&file, config.c_str(), "r");

		if (file)
		{
			char buf[14] = { 0 };

			fread((void*)buf, 1, 13, file);
			fclose(file);

			if (strlen(buf))
				memcpy((void*)clantag, buf, 14);
		}
		else
		{
			FILE* f = nullptr;
			fopen_s(&f, config.c_str(), "w");
			if (f)
			{
				fputs(clantag, f);
				fclose(f);
			}
		}
	}

	DWORD  next  = 0;
	size_t index = 0;

	constexpr size_t ct_count = 14;

	char buffer[ct_count + 1] = { 0 }; // max clantag length

	size_t length = strlen(clantag);
	size_t count  = 14 + length;

	for (;;)
	{
		if (Engine_IsInGame(g_engine) && *g_localplayer)
		{
			if (GetAsyncKeyState(VK_DELETE))
			{
				char empty = '\0';
				SendClantagChanged(&empty, &empty);

				ExitCheat();
			}

			if (_GetTickCount() > next)
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
		}
		else
		{
			if (GetAsyncKeyState(VK_DELETE))
				ExitCheat();
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
