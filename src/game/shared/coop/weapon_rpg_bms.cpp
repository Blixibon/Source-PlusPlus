#include "cbase.h"
#include "weapon_rpg.h"

#ifdef CLIENT_DLL
#define CBMSWeaponRPG C_BMSWeaponRPG
#endif

class CBMSWeaponRPG : public CWeaponRPG
{
	DECLARE_CLASS(CBMSWeaponRPG, CWeaponRPG);
public:
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	bool	Deploy(void);

	virtual void 			GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
};

//=============================================================================
// RPG Weapon
//=============================================================================

LINK_ENTITY_TO_CLASS(weapon_rpg_bms, CBMSWeaponRPG);

PRECACHE_WEAPON_REGISTER(weapon_rpg_bms);

//IMPLEMENT_SERVERCLASS_ST( CHL1WeaponRPG, DT_HL1WeaponRPG )
//END_SEND_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED(BMSWeaponRPG, DT_BMSWeaponRPG);

BEGIN_DATADESC(CBMSWeaponRPG)
END_DATADESC();


BEGIN_NETWORK_TABLE(CBMSWeaponRPG, DT_BMSWeaponRPG)
#ifdef CLIENT_DLL
#else
#endif
END_NETWORK_TABLE();


BEGIN_PREDICTION_DATA(CBMSWeaponRPG)
#ifdef CLIENT_DLL
#endif
END_PREDICTION_DATA();

bool CBMSWeaponRPG::Deploy(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (pOwner)
	{
		CBaseViewModel *pVM = pOwner->GetViewModel();
		pVM->SetBodygroup(0, 1);
	}

	return BaseClass::Deploy();
}

void CBMSWeaponRPG::GetControlPanelInfo(int nPanelIndex, const char*& pPanelName)
{
	if (nPanelIndex == 0)
	{
		pPanelName = "bms_rpg_screen";
	}
}
