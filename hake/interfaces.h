#pragma once

#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#define M_PI_F		((float)(M_PI))	// Shouldn't collide with anything.

#ifndef RAD2DEG
#define RAD2DEG( x  )  ( (float)(x) * (float)(180.f / M_PI_F) )
#endif

#ifndef DEG2RAD
#define DEG2RAD( x  )  ( (float)(x) * (float)(M_PI_F / 180.f) )
#endif

#define MOVETYPE_NOCLIP 8
#define MOVETYPE_LADDER 9

#define FL_ONGROUND (1 << 0)
#define	FL_WATERJUMP (1 << 2)// player jumping out of water
#define	FL_INWATER (1 << 9)// In water
#define	FL_SWIM	(1<<11)	// Changes the SV_Movestep() behavior to not need to be on ground (but stay in water)

#define IN_ATTACK		(1 << 0)
#define IN_JUMP			(1 << 1)
#define IN_DUCK			(1 << 2)
#define IN_FORWARD		(1 << 3)
#define IN_BACK			(1 << 4)
#define IN_USE			(1 << 5)

class CBaseEntity;
class CHlClient;

class QAngle
{
public:
	float x, y, z;
};
class Vector
{
public:
	float x, y, z;
};

float angle_diff(float destAngle, float srcAngle)
{
	float delta = fmodf(destAngle - srcAngle, 360.0f);
	if (destAngle > srcAngle)
	{
		if (delta >= 180)
			delta -= 360;
	}
	else
	{
		if (delta <= -180)
			delta += 360;
	}
	return delta;
}
float normalize_angle(float angle)
{
	if (!std::isfinite(angle))
		return 0.0f;

	return std::remainder(angle, 360.0f);
}
float vector_length(Vector v)
{
	return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}
float vector_length_square(Vector v)
{
	return (v.x*v.x + v.y*v.y + v.z*v.z);
}
float vector_dot_product(Vector v1, Vector v2)
{
	return(v1.x*v2.x + v1.y*v2.y + v1.z*v2.z);
}
QAngle vector_angles(Vector forward)
{
	QAngle angles;

	angles.z = 0;

	if (forward.y == 0 && forward.x == 0)
	{
		angles.y = 0.0f;
		if (forward.z > 0)
			angles.x = -90.f;
		else
			angles.x = 90.f;
	}
	else
	{
		angles.y = (atan2f(forward.y, forward.x) * 180.0f / M_PI_F);
		if (angles.y < 0)
			angles.y += 360.0f;

		angles.x = (atan2f(-forward.z, sqrtf(forward.x * forward.x + forward.y * forward.y)) * 180.0f / M_PI_F);
		if (angles.x < 0)
			angles.x += 360.0f;
	}

	return angles;
}
Vector angle_forward_vector(QAngle angles)
{
	Vector fw;
	float sp, sy, cp, cy;

	float x = DEG2RAD(angles.x);
	float y = DEG2RAD(angles.y);

	sp = std::sinf(x);
	cp = std::cosf(x);

	sy = std::sinf(y);
	cy = std::cosf(y);

	fw.x = cp * cy;
	fw.y = cp * sy;
	fw.z = -sp;
	return fw;
}
float getfov(QAngle my_view, QAngle aim_ang)
{
	if (my_view.x == aim_ang.x && my_view.y == aim_ang.y)
		return 0.0f;

	Vector aim = angle_forward_vector(my_view);
	Vector ang = angle_forward_vector(aim_ang);

	return RAD2DEG(std::acosf(vector_dot_product(aim, ang) / vector_length_square(aim)));
}

class CUserCmd
{//size = 84
public:
	virtual ~CUserCmd() { };	// 0
	int command_number;			// 4
	int tick_count;				// 8
	QAngle viewangles;			// 12 - 20
	float forwardmove;			// 24
	float sidemove;				// 28
	float upmove;				// 32
	int buttons;				// 36
	byte impulse;				// 40
	char pad0[10];				// 41 - 51
	int random_seed;			// 52
	short mousedx;				// 56
	short mousedy;				// 58
	bool hasbeenpredicted;		// 60
	int pad1[5];				// 64 - 84
};

using CRC32_t = unsigned int;
using GetChecksumFn = CRC32_t(__thiscall*)(CUserCmd*);

class CInput
{
public:
	struct VerifiedCmd
	{
		CUserCmd cmd;
		CRC32_t crc;
	};

	CUserCmd* GetUserCmd(int sequence_num)
	{
		auto m_pCommands = *(CUserCmd**)(this + 196);
		return &(m_pCommands[sequence_num % 90]);
	}
	VerifiedCmd* GetVerifiedCmd(int sequence_num)
	{
		auto m_pVerifiedCommands = *(VerifiedCmd**)(this + 200);
		return &(m_pVerifiedCommands[sequence_num % 90]);
	}
};

struct UtlVector_t
{
	PVOID* m_pMemory;
	int m_nAllocationCount;
	int m_nGrowSize;
	int m_Size;
	PVOID* m_pElements;
};
class CVar
{
public:
	CVar()
	{
		memset(this, 0, sizeof(CVar));
	}
	void* vtable;					//0x0000
	CVar* pNext;					//0x0004 
	__int32 bRegistered;			//0x0008 
	const char* pszName;			//0x000C 
	const char* pszHelpString;		//0x0010 
	__int32 nFlags;					//0x0014 
	void* icvarVtable;				//0x0018
	CVar* pParent;					//0x001C 
	const char* cstrDefaultValue;	//0x0020 
	const char* cstrValue;			//0x0024 
	__int32 stringLength;			//0x0028
	float fValue;					//0x002C 
	__int32 nValue;					//0x0030 
	__int32 bHasMin;				//0x0034 
	float fMinVal;					//0x0038 
	__int32 bHasMax;				//0x003C 
	float fMaxVal;					//0x0040 
	UtlVector_t utlvecCallbacks;	//0x0044
};
class ICVar
{
public:
	void RegisterConCommand(CVar* pCVar)
	{
		typedef void(__thiscall* OriginalFn)(void*, CVar*);
		getvfunc<OriginalFn>(this, 12)(this, pCVar);
	}
};
using CvarConstructorFn = void(__thiscall*)(CVar* cvar, const char *pName, const char *pDefaultValue, int flags, const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax);

class CEngineClient
{
public:
	int GetLocalPlayer(void)
	{
		typedef int(__thiscall* OriginalFn)(void*);
		return getvfunc<OriginalFn>(this, 12)(this);
	}
};

class CEntityList
{
public:
	CBaseEntity * GetClientEntity(int entnum)
	{
		typedef CBaseEntity* (__thiscall* OriginalFn)(PVOID, int);
		return getvfunc<OriginalFn>(this, 3)(this, entnum);
	}
};
