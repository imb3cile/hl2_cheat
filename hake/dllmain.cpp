#include <Windows.h>
#include <cmath>
#include <iostream>
#include <string>

using byte = unsigned char;
using uint = unsigned int;

#include "vfuncs.h"
#include "interfaces.h"
#include "ModuleData.h"


// interface and function pointers
CHlClient* g_pHlClient = nullptr;
CEngineClient* g_pEngineClient = nullptr;
CEntityList* g_pEntityList = nullptr;
CInput* g_pInput = nullptr;
GetChecksumFn GetChecksum = nullptr;

// create move hook for bunnyhopping
VMTHook* g_pClientHook = nullptr;
using CreateMoveFn = void(__thiscall*)(PVOID, int, float, bool);
CreateMoveFn oCreateMove = nullptr;
void __stdcall hkdCreateMove(int sequence_number, float input_sample_frametime, bool active)
{
	// doing anything with choking packets is absolutely useless imho
	// but if you still wanna play around with it heres how to get the pointer

	/*bool** basePointer;
	__asm mov basePointer, ebp;
	auto pbSendPacket = *basePointer - 1;*/

	oCreateMove(g_pHlClient, sequence_number, input_sample_frametime, active);

	auto pLocalPlayer = g_pEntityList->GetClientEntity(g_pEngineClient->GetLocalPlayer());

	if (!pLocalPlayer)
		return;

	auto pCmd = g_pInput->GetUserCmd(sequence_number);
	byte movetype = *(byte*)((DWORD)pLocalPlayer + 0x74);
	int flags = *(int*)((DWORD)pLocalPlayer + 0x34C);
	bool onground = (flags & FL_ONGROUND) != 0;
	bool inwater = (flags & FL_INWATER) != 0;
	Vector velocity = *(Vector*)((DWORD)pLocalPlayer + 0xF0);

	if (movetype == MOVETYPE_NOCLIP || movetype == MOVETYPE_LADDER)
		return;

	if (GetAsyncKeyState(VK_MBUTTON) & 0x8000)
	{
		if (pCmd->command_number % 2)
		{
			pCmd->buttons &= ~IN_JUMP;
			pCmd->buttons |= IN_USE;
		}
		else
		{
			pCmd->buttons &= ~IN_USE;
			pCmd->buttons |= IN_JUMP;
		}
	}
	else if (pCmd->buttons & IN_JUMP)
	{
		static bool lastground = onground;
		if (inwater && pCmd->buttons & IN_DUCK)
		{
			lastground = onground;
			return;
		}

		float speed = vector_length(velocity);
		bool moving = speed > 0.1f;
		bool backmove = false;
		if (moving)
		{
			QAngle moveang = vector_angles(velocity);
			moveang.x = 0.f;
			QAngle viewang = pCmd->viewangles;
			viewang.x = 0.f;
			float delta = getfov(viewang, moveang);
			backmove = (delta > 90.f);
		}

		if (!onground)
		{
			lastground = false;
			pCmd->buttons &= ~IN_JUMP;
		}
		else if (!lastground)
		{
			lastground = true;
			if (!backmove)
				pCmd->forwardmove = -0.1f;
		}
	}

	g_pInput->GetVerifiedCmd(sequence_number)->crc = GetChecksum(pCmd);
}

// the setup function for all this dumb bs
DWORD __stdcall SetupThread()
{
	// getting client.dll stuff
	ModuleData client("client.dll");
	if (!client.valid())
		return 0;

	g_pHlClient = client.capture_interface<CHlClient>("VClient017");
	g_pEntityList = client.capture_interface<CEntityList>("VClientEntityList003");
	GetChecksum = client.pattern_scan<GetChecksumFn>("55 8B EC 51 56 8D 45 FC");
	g_pInput = client.pattern_scan<CInput*>("8B 0D ? ? ? ? FF 75 10 D9 45 0C", 2, 2);

	if (!g_pInput || !GetChecksum || !g_pEntityList || !g_pHlClient)
		return 0;

	// getting engine.dll stuff
	ModuleData engine("engine.dll");
	if (!engine.valid())
		return 0;

	g_pEngineClient = engine.capture_interface<CEngineClient>("VEngineClient014");
	if (!g_pEngineClient)
		return 0;

	// setting up our hook
	g_pClientHook = new VMTHook(g_pHlClient);
	oCreateMove = g_pClientHook->hook<CreateMoveFn>(&hkdCreateMove, 21);

	MessageBeep(MB_OK);
	return 0;
}

BOOL WINAPI DllMain(HMODULE hDll, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SetupThread, NULL, NULL, NULL);

	return 1;
}

