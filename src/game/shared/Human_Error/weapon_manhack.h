//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Alien Controllers from HL1 now in updated form
//
//=============================================================================//


#ifndef WEAPON_MANHACK
#define WEAPON_MANHACK


#include "cbase.h"
#include "coop/weapon_coop_basehlcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeapon_Manhack C_Weapon_Manhack
#endif

class CWeapon_Manhack : public CWeaponCoopBaseHLCombat
{
public:
	DECLARE_CLASS( CWeapon_Manhack, CWeaponCoopBaseHLCombat);
	DECLARE_PREDICTABLE();
	DECLARE_NETWORKCLASS();

	CWeapon_Manhack();

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_MANHACK; }

	//void			Spawn( void );
	void			Precache( void );

	void			ItemBusyFrame( void );
	void			ItemPostFrame( void );
	void			PrimaryAttack( void );
	void			SecondaryAttack( void );

	void			EnableManhackSubModel(bool bEnable);

	bool			Deploy(void);
	bool			Reload(void);

	void			DecrementAmmo( CBaseCombatCharacter *pOwner );

	void			WeaponIdle(void);
	bool			HasAnyAmmo(void);

	void			WeaponSwitch( void );

	//bool			FindVehicleManhack();
#ifndef CLIENT_DLL
	void			Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	bool			CreateControllableNPCManhack( CBasePlayer *pPlayer );
	void			DriveControllableManhack();
	void			UpdateControllerPanel();
#else
	virtual int				GetWeaponRenderTargetCount() { return 1; }
	virtual bool			GetWeaponRenderTarget(int iWhich, weaponrendertarget_t& data, const CNewViewSetup& mainView);
	virtual void			WeaponRT_StartRender3D(int iWhich);
	virtual void			WeaponRT_FinishRender3D(int iWhich);
	virtual void			WeaponRT_StartRender2D(int iWhich);
#endif
	bool			HasNPCManhack();
	bool			HasFreeSlot();
	int				NumberOfManhacks();

	void			CallManhacksBack();
	void			TellManhacksToGoThere();


private:

	//EHANDLE			m_hManhack;

	//Animation booleans
	CNetworkVar(bool, m_bIsDrawing);
	CNetworkVar(bool, m_bIsDoingShit);
	CNetworkVar(bool, m_bIsDoingShitToo);
	CNetworkVar(bool, m_bIsDoingController);
	CNetworkVar(bool, m_bSpawnSomeMore);

	bool			m_bSkip;
	bool			m_bRedraw;
	bool			m_bHoldingSpawn;

	bool			m_bHasAmmo;
	bool			m_bHasFreeSlot;

#ifndef CLIENT_DLL
	bool			m_bToggleCallback;
	int				m_iManhackHintTimeShown;
	bool			m_bHadControllable;		//If Manhacks used to be controllable, but now they are not, other the other way around
#endif // !CLIENT_DLL

	int				m_iLastNumberOfManhacks;

	//EHANDLE			m_hScreen;

	CNetworkVar( int, m_iManhackDistance );

	DECLARE_DATADESC();
};

#endif //WEAPON_MANHACK
