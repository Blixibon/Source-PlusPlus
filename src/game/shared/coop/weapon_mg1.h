//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Projectile shot from the AR2 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	WEAPONMG1_H
#define	WEAPONMG1_H

#include "basegrenade_shared.h"
#include "weapon_coop_basemachinegun.h"

#ifdef CLIENT_DLL
#define CWeaponMG1 C_WeaponMG1
#endif

class CWeaponMG1 : public CWeaponCoopMachineGun
{
public:
	DECLARE_CLASS(CWeaponMG1, CWeaponCoopMachineGun);

	CWeaponMG1();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_MG1; }

	void	ItemPostFrame(void);
	void	Precache(void);

	void	SecondaryAttack(void);
	//void	DelayedAttack(void);

	const char *GetTracerType(void) { return "AR2Tracer"; }
	void	AddViewKick(void);

#ifndef CLIENT_DLL
	int		CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	void	FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles);
	void	FireNPCSecondaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles);
	void	Operator_ForceNPCFire(CBaseCombatCharacter  *pOperator, bool bSecondary);
	void	Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
#endif

	int		GetMinBurst(void) { return 5; }
	int		GetMaxBurst(void) { return 12; }
	float	GetFireRate(void) { return 0.06f; }

	bool	CanHolster(void);
	bool	Reload(void);

	Activity	GetPrimaryAttackActivity(void);

	void	DoImpactEffect(trace_t &tr, int nDamageType);

	virtual const Vector& GetBulletSpread(void)
	{
		static Vector cone;

		cone = VECTOR_CONE_4DEGREES;

		return cone;
	}

	const WeaponProficiencyInfo_t *GetProficiencyValues();

protected:

	//float					m_flDelayedFire;
	//bool					m_bShotDelayed;
	//int						m_nVentPose;

	//DECLARE_ACTTABLE();

#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif
};


#endif	//WEAPONMG1_H
