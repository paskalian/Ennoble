#include <Windows.h>
#include "Globals.hpp"

void MainThread(HMODULE hModule)
{
	RGlobals.Start();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MainThread, hModule, NULL, NULL);

		break;
	}
	}
	return TRUE;
}
