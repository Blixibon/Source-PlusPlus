#ifndef BMS_UTIL_H
#define BMS_UTIL_H
#include "gamerules.h"
#ifdef GAME_DLL
#include "gametypes.h"
#include "globalstate.h"

class CAI_PlayerAlly;
#endif

#ifdef GAME_DLL
inline bool IsBMSMap()
{
	return g_pGameTypeSystem->GetCurrentGameType() == MOD_BMS;
}

inline bool IsPreDisaster()
{
	return GlobalEntity_GetState("predisaster") == GLOBAL_ON;
}

#else
inline bool IsBMSMap()
{
	return (!Q_strnicmp(g_pGameRules->MapName(), "bm_", 3));
}
#endif

struct RndFlexData
{
	int index;
	float flvalue;
	bool bValid;

	RndFlexData(int i, float v) : index(i), flvalue(v), bValid(true)
	{}

	RndFlexData() : index(-1), flvalue(0), bValid(false)
	{}

	RndFlexData(bool v) : bValid(v)
	{}

	DECLARE_SIMPLE_DATADESC()
};



static const char *g_szRandomFlexControls[] = {
	"cheek_depth",
	"cheek_fat_min",
	"cheek_fat_max",
	"chin_butt",
	"chin_width",
	"chin_height",
	"neck_size",
	"lowlip_size",
	"uplip_size",
	"mouth_w_max",
	"mouth_w_min",
	"mouth_h_min",
	"mouth_depth",
	"nose_h_max",
	"nose_h_min"
	"nose_w_min",
	"nose_angle",
	"nose_d_max",
	"nose_tip",
	"nost_height",
	"nost_width",
	//"hairline_puff",
	"head_height",
	"head_w_min",
	"head_w_max",
	"neck_size",
	"ears_angle",
	"ears_height",
	"eyes_ang_min",
	"eyes_ang_max",
	"eyes_height",
	"jaw_depth",
};

#define NUM_RND_HEAD_FLEXES ARRAYSIZE(g_szRandomFlexControls)

#endif //!BMS_UTIL_H