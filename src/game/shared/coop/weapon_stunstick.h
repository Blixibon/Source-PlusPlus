//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is technically a Mapbase addition, but it's just weapon_stunstick's class declaration.
// 			All actual changes are still nested in #ifdef MAPBASE.
//
//=============================================================================//

#ifndef WEAPON_STUNSTICK_H
#define WEAPON_STUNSTICK_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_coop_basebludgeon.h"
#include "utllinkedlist.h"
#ifdef CLIENT_DLL
#include "iviewrender_beams.h"
#endif // CLIENT_DLL


#define	STUNSTICK_RANGE				75.0f
#define	STUNSTICK_REFIRE			0.8f
#define	STUNSTICK_BEAM_MATERIAL		"sprites/lgtning.vmt"
#define STUNSTICK_GLOW_MATERIAL		"sprites/light_glow02_add"
#define STUNSTICK_GLOW_MATERIAL2	"effects/blueflare1"
#define STUNSTICK_GLOW_MATERIAL_NOZ	"sprites/light_glow02_add_noz"

#ifdef CLIENT_DLL
#define CWeaponStunStick C_WeaponStunStick

#include "flashlighteffect.h"

#endif

#ifdef CLIENT_DLL
class CWeaponStunStick : public CWeaponCoopBaseBludgeon, public IBeamRemovedCallback
#else
class CWeaponStunStick : public CWeaponCoopBaseBludgeon
#endif // CLIENT_DLL
{
	DECLARE_CLASS( CWeaponStunStick, CWeaponCoopBaseBludgeon);
	
public:

	CWeaponStunStick();
	~CWeaponStunStick();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_STUNSTICK; }

#ifdef CLIENT_DLL
	virtual int				DrawModel( int flags );
	virtual void			ClientThink( void );
	virtual void			OnDataChanged( DataUpdateType_t updateType );
	virtual RenderGroup_t	GetRenderGroup( void );
	virtual void			ViewModelDrawn( C_BaseViewModel *pBaseViewModel );

	virtual void OnBeamFreed(Beam_t* pBeam);

	// This is called to do the actual muzzle flash effect.
	//void ProcessMuzzleFlashEvent();
	void Simulate( void );

protected:
	float m_flLastMuzzleFlashTime;

	CFlashlightEffectBase *m_pStunstickLight;

public:

#endif

	virtual void Precache();

	void		Spawn();

	float		GetRange( void )		{ return STUNSTICK_RANGE; }
	float		GetFireRate( void )		{ return STUNSTICK_REFIRE; }


	bool		Deploy( void );
	bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	
	void		Drop( const Vector &vecVelocity );
	void		ImpactEffect( trace_t &traceHit );
	void		SetStunState( bool state );
	bool		GetStunState( void );

#ifndef CLIENT_DLL
	void		Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	int			WeaponMeleeAttack1Condition( float flDot, float flDist );
#endif
	
	float		GetDamageForActivity( Activity hitActivity );

	virtual void	ItemPostFrame(void);
	void			SecondaryAttack(void);
	void			Hit(trace_t& traceHit, Activity nHitActivity);
private:
	// Fields for chargeup attack
	CNetworkVar(float, m_flLastChargeTime);
	CNetworkVar(float, m_flChargeAmount);

	CWeaponStunStick( const CWeaponStunStick & );

private:

	bool	CalcInSwingState();
	bool	InSwing(void); // Moved from client to be shared with client and server

#ifdef CLIENT_DLL

	#define	NUM_BEAM_ATTACHMENTS	9

	struct stunstickBeamInfo_t
	{
		int IDs[2];		// 0 - top, 1 - bottom
	};

	stunstickBeamInfo_t		m_BeamAttachments[NUM_BEAM_ATTACHMENTS];	// Lookup for arc attachment points on the head of the stick
	int						m_BeamCenterAttachment;						// "Core" of the effect (center of the head)
	int						m_BeamCenterAttachmentWorld;

	void	SetupAttachmentPoints( void );
	void	DrawFirstPersonEffects( void );
	void	DrawThirdPersonEffects( void );
	void	DrawNPCEffects( void );
	void	DrawEffects( bool bWorldModel );
	bool	m_bSwungLastFrame;

	#define	FADE_DURATION	0.25f

	float	m_flFadeTime;

	CUtlLinkedList<Beam_t*> m_beamsView;
	CUtlLinkedList<Beam_t*> m_beamsWorld;
#endif

	CNetworkVar( bool, m_bActive );
	CNetworkVar( bool, m_bInSwing );
};

#endif // WEAPON_STUNSTICK_H
