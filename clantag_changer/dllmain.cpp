#include <Windows.h>
#include <iostream>
#include <vector>
#include "utils.h"
#include "winapi.h"

struct clantag_t
{
	std::string tag;
	size_t time;
};

using SendClantagChangedFn = void(__fastcall*)(const char*, const char*);
using Engine_IsInGameFn    = bool(__fastcall*)(void*);

SendClantagChangedFn SendClantagChanged = nullptr;
Engine_IsInGameFn    Engine_IsInGame = nullptr;

HMODULE g_dll         = nullptr;
byte*   g_engine      = nullptr;
byte**  g_localplayer = nullptr;

std::vector<clantag_t> g_clantags;

void ExitCheat()
{
	if (g_dll)
		_FreeLibraryAndExitThread(g_dll, 0);
	else
		_ExitThread(0);
}

bool IsInjectedToCSGO()
{
	char filename[MAX_PATH];
	_GetModuleFileNameA(NULL, filename, MAX_PATH);

	// make sure we're actually injected @ csgo
	return (std::string(filename).find(strenc("\\steamapps\\")) != std::string::npos
		 && std::string(filename).find(strenc("\\Counter-Strike Global Offensive\\")) != std::string::npos);
}

bool WaitForCSGO(DWORD waitTime)
{
	DWORD start = _GetTickCount();

	// wait until csgo has fully loaded
	while (!_GetModuleHandleA(charenc("serverbrowser.dll")))
	{
		if (_GetTickCount() - start > waitTime)
			return false;

		_Sleep(100);
	}

	return true;
}

bool GetConfigPath(char* path, const char* configName, size_t size)
{
	char filename[MAX_PATH] = { 0 };

	_GetModuleFileNameA(NULL, filename, MAX_PATH);

	size_t count;
	for (count = strlen(filename); count != 0; --count)
	{
		if (filename[count] == '\\')
		{
			++count;
			break;
		}
	}

	if (count && (count + strlen(configName) <= size))
	{
		memcpy(path, filename, count);
		memcpy(path + count, configName, strlen(configName));
		return true;
	}
	return false;
}

void LoadOrCreateConfig(const char* configName)
{
	char  path[MAX_PATH] = { 0 };
	char* buffer = (char*)charenc(":300\ns:300\nsa:300\nsam:300\nsamp:300\nsampl:300\nsample:300");
	size_t end = strlen(buffer);

	if (GetConfigPath(path, configName, MAX_PATH))
	{
		FILE* file = nullptr;
		fopen_s(&file, path, "r");

		if (file)
		{
			fseek(file, 0, SEEK_END);
			long size = ftell(file);
			fseek(file, 0, SEEK_SET);

			if (size > 0)
			{
				end = size + 1;
				buffer = new char[end];
				memset(buffer, 0, end);

				fread(buffer, 1, size, file);
				fclose(file);
			}
		}
		else
		{
			fopen_s(&file, path, "w");
			if (file)
			{
				fputs(buffer, file);
				fclose(file);
			}
		}

		std::vector<std::string> clantags;

		size_t off = 0,
			   i   = 0;

		for (; ; )
		{
			if (i == end)
			{
				if (i != off)
					clantags.emplace_back(std::string(buffer).substr(off, i - off));
				break;
			}
			if (buffer[i] == '\n')
			{
				clantags.emplace_back(std::string(buffer).substr(off, i - off));
				off = i + 1;
			}
			++i;
		}

		for (const auto& tag : clantags)
		{
			size_t index = tag.find_last_of(':');
			if (index != tag.length())
			{
				clantag_t ct;
				ct.tag    = tag.substr(0, index);
				auto time = tag.substr(index + 1);

				if (!time.empty())
					ct.time = std::stoul(time);
				else
					ct.time = 0;

				g_clantags.emplace_back(ct);
			}
		}
	}
}

void MainThread()
{
	size_t next = 0,
	       index = 0;

	for (;;)
	{
		if (Engine_IsInGame(g_engine) && *g_localplayer && *(int*)(*g_localplayer + 0xF4 /*m_iTeamNum*/))
		{
			if (GetAsyncKeyState(VK_DELETE))
			{
				char empty = '\0';
				SendClantagChanged(&empty, &empty);
				ExitCheat();
			}

			size_t cur = _GetTickCount();
			if (cur > next)
			{
				const auto& ct = g_clantags[index];

				SendClantagChanged(ct.tag.c_str(), ct.tag.c_str());
				next = cur + ct.time;

				++index;
				if (index >= g_clantags.size())
					index = 0;
			}
		}
		else if (GetAsyncKeyState(VK_DELETE))
			ExitCheat();

		_Sleep(10);
	}
}

DWORD __stdcall Initialize(LPVOID lpThreadParameter)
{
	if (!IsInjectedToCSGO())
		ExitCheat();

	if (!WaitForCSGO(120 * 1000))
		ExitCheat();

	const auto clientEntry = utils::GetModuleEntry(hash("client_panorama.dll"));
	const auto engineEntry = utils::GetModuleEntry(hash("engine.dll"));

	if (!clientEntry || !engineEntry)
		ExitCheat();

	DWORD dwLocalPlayer   = utils::ModulePatternScan(clientEntry, charenc("8B 0D ? ? ? ? 83 FF FF 74 07"));
	DWORD dwInterfaceList = utils::ModulePatternScan(engineEntry, charenc("8B 35 ? ? ? ? 57 85 F6 74 38 8B 7D 08"));
	SendClantagChanged    = (SendClantagChangedFn)utils::ModulePatternScan(engineEntry, charenc("53 56 57 8B DA 8B F9 FF"));

	if (!dwLocalPlayer || !dwInterfaceList || !SendClantagChanged)
		ExitCheat();

	g_engine      =  (byte*)utils::GetInterface(dwInterfaceList + 2, hash("VEngineClient0"), 14);
	g_localplayer = *(byte***)(dwLocalPlayer + 2);

	if(!g_engine || !g_localplayer)
		ExitCheat();
		
	Engine_IsInGame = (Engine_IsInGameFn)(*(unsigned long**)g_engine)[26];

	if (!Engine_IsInGame)
		ExitCheat();

	LoadOrCreateConfig(charenc("clantag.cfg"));
	MainThread();

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
		_CreateThread(NULL, 0, Initialize, nullptr, 0, nullptr);
	}
	return TRUE;
}
