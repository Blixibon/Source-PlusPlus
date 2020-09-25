//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	weapon_manhack, create a manhack (vehicle_manhack) and control it
//			CONTROL. WE HAVE IT.
//
//=============================================================================//



#include "cbase.h"
#include "weapon_manhack.h"
#include "gamerules.h"
#include "ammodef.h"
#include "in_buttons.h"

#include "engine/IEngineSound.h"

#include "npcevent.h"

#include "laz_player_shared.h"

#ifndef CLIENT_DLL
#include "items.h"
#include "soundent.h"
#include "globalstate.h"
#include "npc_manhack.h"
#include "Human_Error/vehicle_manhack.h"
#else
#include "view_shared.h"
#include "viewrender.h"
#include "peter/laz_render_targets.h"
#include "materialsystem/itexture.h"
#include "Human_Error/c_manhack_screen.h"
#endif // !CLIENT_DLL



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define SCREEN_OVERLAY_MATERIAL "engine/writez" 

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CWeapon_Manhack)
//	DEFINE_FIELD( m_hManhack,				FIELD_EHANDLE),
DEFINE_FIELD(m_bIsDrawing, FIELD_BOOLEAN),
DEFINE_FIELD(m_bIsDoingShit, FIELD_BOOLEAN),
DEFINE_FIELD(m_bIsDoingShitToo, FIELD_BOOLEAN),
DEFINE_FIELD(m_bRedraw, FIELD_BOOLEAN),
DEFINE_FIELD(m_bHasAmmo, FIELD_BOOLEAN),
DEFINE_FIELD(m_bIsDoingController, FIELD_BOOLEAN),
DEFINE_FIELD(m_bSpawnSomeMore, FIELD_BOOLEAN),
DEFINE_FIELD(m_bSkip, FIELD_BOOLEAN),
DEFINE_FIELD(m_bHoldingSpawn, FIELD_BOOLEAN),
DEFINE_FIELD(m_iLastNumberOfManhacks, FIELD_INTEGER),
DEFINE_FIELD(m_iManhackDistance, FIELD_INTEGER),
#ifndef CLIENT_DLL
DEFINE_FIELD(m_bToggleCallback, FIELD_BOOLEAN),
DEFINE_FIELD(m_bHadControllable, FIELD_BOOLEAN),
DEFINE_FIELD(m_iManhackHintTimeShown, FIELD_INTEGER),
#endif
END_DATADESC();

BEGIN_PREDICTION_DATA(CWeapon_Manhack)
#ifdef CLIENT_DLL

#endif // CLIENT_DLL
END_PREDICTION_DATA();

IMPLEMENT_NETWORKCLASS_ALIASED(Weapon_Manhack, DT_Weapon_Manhack);

BEGIN_NETWORK_TABLE(CWeapon_Manhack, DT_Weapon_Manhack)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bIsDrawing)),
RecvPropBool(RECVINFO(m_bIsDoingShit)),
RecvPropBool(RECVINFO(m_bIsDoingShitToo)),
RecvPropBool(RECVINFO(m_bIsDoingController)),
RecvPropBool(RECVINFO(m_bSpawnSomeMore)),
RecvPropInt(RECVINFO(m_iManhackDistance), SPROP_CHANGES_OFTEN),
#else
	SendPropBool(SENDINFO(m_bIsDrawing)),
	SendPropBool(SENDINFO(m_bIsDoingShit)),
	SendPropBool(SENDINFO(m_bIsDoingShitToo)),
	SendPropBool(SENDINFO(m_bIsDoingController)),
	SendPropBool(SENDINFO(m_bSpawnSomeMore)),
	SendPropInt(SENDINFO(m_iManhackDistance), SPROP_CHANGES_OFTEN),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( weapon_manhack, CWeapon_Manhack );
PRECACHE_WEAPON_REGISTER(weapon_manhack);


#ifndef CLIENT_DLL
void CWeapon_Manhack::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	switch( pEvent->event )
	{
		case EVENT_WEAPON_SEQUENCE_FINISHED:
			m_bIsDrawing = false;
			m_bIsDoingShit = false;
			m_bIsDoingShitToo = false;

			if (m_bRedraw)
			{
				m_bRedraw=false;
				Deploy();
			}
			break;

		case EVENT_WEAPON_THROW:
			
			if (HasFreeSlot())
			{
				if (CreateControllableNPCManhack(pOwner))
				{
					DecrementAmmo(pOwner);
					EnableManhackSubModel(false);
					m_bRedraw = true;

					CBasePlayer* pPlayer = ToBasePlayer(pOwner);
					if (pPlayer != NULL && m_iManhackHintTimeShown < 2)
					{
						if (GlobalEntity_GetState("manhacks_not_controllable") == GLOBAL_ON)
							UTIL_HudHintText(pPlayer, "#HLSS_Hint_ManhackSend");
						else
							UTIL_HudHintText(pPlayer, "#HLSS_Hint_ManhackControl");

						m_iManhackHintTimeShown++;
					}
					//						fSpawnedManhack = true;
				}
			}

			m_bToggleCallback = true;

			if (!m_bHoldingSpawn)
			{
				m_bSpawnSomeMore = false;
				m_bIsDoingController = true;
			}
			else
			{
				m_bSpawnSomeMore = true;
				m_bIsDoingController = false;
				m_bRedraw = true;
			}

			break;
		case EVENT_WEAPON_THROW2:
			if (pOwner != NULL && pOwner->GetFlags() & FL_ONGROUND)
			{
				if (GlobalEntity_GetState("manhacks_not_controllable") == GLOBAL_OFF)
				{
					DriveControllableManhack();
				}
				else
				{
					if (m_bToggleCallback)
					{
						TellManhacksToGoThere();
						m_bToggleCallback = false;
					}
					else
					{
						CallManhacksBack();
						m_bToggleCallback = true;
					}
				}
			}
			break;
		case EVENT_WEAPON_THROW3:
			if (m_bToggleCallback)
			{
				TellManhacksToGoThere();
				m_bToggleCallback = false;
			}
			else
			{
				CallManhacksBack();
				m_bToggleCallback = true;
			}
			break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}

#define RETHROW_DELAY	0.5
/*	if( fSpawnedManhack )
	{
		m_flNextPrimaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flNextSecondaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!

	}
	*/
}
#endif
CWeapon_Manhack::CWeapon_Manhack(void)
{
//	m_hManhack = NULL;
	m_bIsDrawing=false;
	m_bIsDoingShit=false;
	m_bIsDoingShitToo=false;
	m_bRedraw=false;
	m_bHasAmmo=true;
	m_bIsDoingController=false;
	m_bSpawnSomeMore=false;
	m_bSkip=true;
#ifndef CLIENT_DLL
	m_iManhackHintTimeShown=0;
	m_bHadControllable = false;
#endif

	m_iLastNumberOfManhacks=-1;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::ItemBusyFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if (pOwner)
	{
		if ( m_bHoldingSpawn && pOwner->m_nButtons & IN_ATTACK )
		{
			m_bHoldingSpawn = true;
		}
		else
		{
			m_bHoldingSpawn = false;
		}
	}

	BaseClass::ItemBusyFrame();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::ItemPostFrame( void )
{
	m_bHasAmmo = false;
	m_bHasFreeSlot = HasFreeSlot();

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if (pOwner)
	{
		if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
		{
			m_bHasAmmo=true;
		}

		if ( pOwner->m_nButtons & IN_ATTACK )
		{
			m_bHoldingSpawn = true;
		}
		else
		{
			m_bHoldingSpawn = false;
		}
	}

	if (pOwner && !m_bIsDoingShit && !m_bIsDrawing && !m_bIsDoingShitToo && HasAnyAmmo() )
	{
			
		if( pOwner->m_nButtons & IN_ATTACK )
		{
			if ( m_bHasAmmo && (m_bHasFreeSlot) && !m_bIsDoingController)
			{
				PrimaryAttack();
			}
			else 
			{
				SecondaryAttack();
			}

			return;
		} 
		else
		{
			if ( pOwner->m_nButtons & IN_ATTACK2 && m_bHasFreeSlot )
			{
				//SendWeaponAnim(ACT_SLAM_DETONATOR_THROW_DRAW);
				//m_bIsDoingShitToo=true;

				CBaseCombatCharacter *pOwner  = GetOwner();
				if (pOwner && pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0 && !m_bSpawnSomeMore )
				{
					m_bSpawnSomeMore=true;
					m_bIsDoingShitToo=true;
				} 
				else if ( HasNPCManhack() && m_bSpawnSomeMore)
				{
					m_bSpawnSomeMore=false;
					//m_bIsDoingShitToo=true;
					//m_bIsDoingController=true;
					m_bIsDoingShitToo=true;
					m_bRedraw=true;
					m_bSkip = true;
				}
			}
		} 
	}

	WeaponIdle( );
}

void CWeapon_Manhack::TellManhacksToGoThere() 
{
#ifndef CLIENT_DLL
	CLaz_Player* pPlayer = ToLazuulPlayer(GetOwner());
	if (pPlayer != NULL)
	{
		pPlayer->TellManhacksToGoThere(4.f);
	}
#endif // !CLIENT_DLL
}

void CWeapon_Manhack::CallManhacksBack()
{
#ifndef CLIENT_DLL
	CLaz_Player* pPlayer = ToLazuulPlayer(GetOwner());
	if (pPlayer != NULL)
	{
		pPlayer->CallManhacksBack(4.f);
	}
#endif // !CLIENT_DLL
}

void CWeapon_Manhack::Precache( void )
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	UTIL_PrecacheOther("npc_manhack");
	UTIL_PrecacheOther("vehicle_manhack");
#endif // !CLIENT_DLL
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::PrimaryAttack()
{
	if ( !m_bHasFreeSlot )
	{
		return;
	}

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
	{ 
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );;

	if ( !pPlayer )
		return;

	m_bIsDoingShit = true;

	// Note that this is a primary attack and prepare the grenade attack to pause.
	SendWeaponAnim( ACT_VM_THROW );

	DevMsg("                return true\n");

	// If I'm now out of ammo, switch away
	/*if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}*/
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::SecondaryAttack()
{
	if ( !HasNPCManhack() )
	{
		return;
	}

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
	{ 
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );;

	if ( !pPlayer )
		return;


	m_bIsDoingShit = true;

	SendWeaponAnim( ACT_SLAM_DETONATOR_DETONATE );
}

#ifndef CLIENT_DLL
extern ConVar	manhack_small_distances_max_distance;
extern ConVar	manhack_small_distances_max_height;
extern ConVar	manhack_max_distance;

int GetManhackDistance(CBaseEntity* pOrigin, CNPC_Manhack *pManhack)
{
	if (pManhack != NULL && pOrigin) //!GetDriver()) 
	{
		int iDist = -1;

		if (GlobalEntity_GetState("manhacks_use_short_distances") == GLOBAL_ON)
		{
			Vector vecDistance = (pManhack->GetAbsOrigin() - pOrigin->GetAbsOrigin());
			float flHeight = vecDistance.z / manhack_small_distances_max_height.GetFloat();
			vecDistance.z = 0.0f;
			vecDistance = vecDistance / manhack_small_distances_max_distance.GetFloat();
			vecDistance.z = flHeight;
			iDist = (int)(vecDistance.Length() * 100.0f);
		}
		else
		{
			iDist = (int)((float)(pManhack->GetAbsOrigin() - pOrigin->GetAbsOrigin()).Length() / manhack_max_distance.GetFloat() * 100.0f);
		}

		return iDist;
	}
	return -1;
}

void CWeapon_Manhack::UpdateControllerPanel()
{
	if (GlobalEntity_GetState("manhacks_not_controllable") == GLOBAL_ON)
	{
		m_iManhackDistance = 100;
		return;
	}

	CLaz_Player* pPlayer = ToLazuulPlayer(GetOwner());
	if (pPlayer)
	{
		m_iManhackDistance = GetManhackDistance(pPlayer, pPlayer->GetCurrentManhack());
	}
}
#else
bool CWeapon_Manhack::GetWeaponRenderTarget(int iWhich, weaponrendertarget_t& data, const CNewViewSetup& mainView)
{
	C_Laz_Player* pPlayer = ToLazuulPlayer(GetOwner());
	if (iWhich == 0 && pPlayer)
	{
		ITexture *pTexture = lazulrendertargets->GetManhackScreenTexture();
		data.m_pRenderTarget = pTexture;

		CNewViewSetup View = mainView;
		View.x = 0;
		View.y = 0;
		View.width = pTexture->GetActualWidth();
		View.height = pTexture->GetActualHeight();
		View.fov = 50.f;
		View.m_flAspectRatio = View.width / View.height;

		C_NPC_Manhack* pManhack = pPlayer->GetCurrentManhack();
		if (pManhack)
		{
			View.origin = pManhack->EyePosition();
			View.angles = pManhack->EyeAngles();
			data.m_bDraw3D = true;
			data.m_bDraw3DSkybox = true;
			data.m_3DViewID = VIEW_MONITOR;
		}
		else
		{
			data.m_bDraw3D = false;
		}

		data.m_View = View;

		data.m_bDraw2D = true;
		data.m_2DPanel = GetManhackScreen()->GetVPanel();
		return true;
	}

	return false;
}

RenderMode_t g_ManhackSavedRenderMode = kRenderNormal;
void CWeapon_Manhack::WeaponRT_StartRender3D(int iWhich)
{
	C_Laz_Player* pPlayer = ToLazuulPlayer(GetOwner());
	if (iWhich == 0 && pPlayer && pPlayer->GetCurrentManhack())
	{
		C_NPC_Manhack* pManhack = pPlayer->GetCurrentManhack();
		g_ManhackSavedRenderMode = pManhack->GetRenderMode();
		pManhack->SetRenderMode(kRenderNone);
	}
}
void CWeapon_Manhack::WeaponRT_FinishRender3D(int iWhich)
{
	C_Laz_Player* pPlayer = ToLazuulPlayer(GetOwner());
	if (iWhich == 0 && pPlayer && pPlayer->GetCurrentManhack())
	{
		C_NPC_Manhack* pManhack = pPlayer->GetCurrentManhack();
		pManhack->SetRenderMode(g_ManhackSavedRenderMode);
	}
}
void CWeapon_Manhack::WeaponRT_StartRender2D(int iWhich)
{
	C_Laz_Player* pPlayer = ToLazuulPlayer(GetOwner());
	if (iWhich == 0 && pPlayer)
	{
		GetManhackScreen()->SetManhackData(m_iManhackDistance, pPlayer->GetManhackCount());
	}
}
#endif // !CLIENT_DLL


void CWeapon_Manhack::WeaponIdle( void )
{
	/*bool hasAmmo = false;
	CBaseCombatCharacter *pOwner  = GetOwner();
	if (pOwner==NULL) return false;
	if (pOwner->GetAmmoCount(m_iSecondaryAmmoType) > 0)
		hasAmmo=true;*/

	// Ready to switch animations?
 
	int	 iAnim;

	//TERO: lets set how many Manhacks we have online
	CBaseCombatCharacter *pOwner  = GetOwner();
	if (pOwner)
	{
		pOwner->SetAmmoCount( NumberOfManhacks(), m_iSecondaryAmmoType );
	}

	if ( (HasNPCManhack() || !m_bHasAmmo) && !m_bSpawnSomeMore ) 
	{
		iAnim=ACT_SLAM_DETONATOR_IDLE;
		m_bIsDoingController=true;
#ifndef CLIENT_DLL
		UpdateControllerPanel();
#endif
		//TERO:

		if (m_bIsDrawing)
			iAnim=ACT_SLAM_DETONATOR_DRAW;
		else if (m_bIsDoingShit)
			iAnim=ACT_SLAM_DETONATOR_DETONATE;
		else if (m_bIsDoingShitToo)
			iAnim=ACT_VM_HOLSTER; //ACT_SLAM_DETONATOR_THROW_DRAW
			
		
		if (HasWeaponIdleTimeElapsed() || m_bSkip)
		{
			SendWeaponAnim( iAnim );
			m_bSkip=false;
		}
	}
	else
	{
#ifndef CLIENT_DLL
		UpdateControllerPanel();
#endif

		iAnim=ACT_VM_IDLE;

		m_bSkip =false;

		if (m_bIsDoingController)
		{
			m_bIsDoingShitToo=true;
			m_bRedraw=true;
			m_bSkip=true;

			m_bIsDoingController=false;
		}
	
		if ( HasWeaponIdleTimeElapsed() || m_bSkip )
		{
			if (m_bIsDrawing)
				iAnim=ACT_VM_DRAW;
			else if (m_bIsDoingShit)
				iAnim=ACT_VM_THROW;
			else if (m_bIsDoingShitToo)
				iAnim=ACT_SLAM_DETONATOR_HOLSTER;

			SendWeaponAnim( iAnim );

		}
	}
		
}

void CWeapon_Manhack::EnableManhackSubModel(bool bEnable)
{
	//SetBodygroup( 0, bEnable );
	if (bEnable)
		DevMsg("weapon_manhack: enabling manhack submodel\n");
	else
		DevMsg("weapon_manhack: disabling manhack: submodel\n");
}

bool CWeapon_Manhack::Deploy( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
	{
		return false;
	}

	// ------------------------------
	// Pick the right draw animation
	// ------------------------------
	int iActivity;

	m_bHasFreeSlot = HasFreeSlot();

	bool bNoDrivable = !HasNPCManhack();

	if ( m_bHasAmmo && m_bHasFreeSlot && (!m_bIsDoingController || bNoDrivable))
	{
		iActivity = ACT_VM_DRAW;
		//EnableManhackSubModel(true);

		CBasePlayer *pPlayer = ToBasePlayer( pOwner );
			if ( pPlayer != NULL)

		m_bIsDoingController = false;
		m_bSpawnSomeMore = true; //TERO: not sure about this
	} 
	else
	{
		m_bIsDoingController = true;
		m_bSpawnSomeMore = false;
		iActivity = ACT_SLAM_DETONATOR_DRAW;
	}

	m_bIsDrawing=true;

#ifndef CLIENT_DLL
	//TERO: this next bit is to make sure the player gets the right hint message if the controllable state has changed
	if (GlobalEntity_GetState("manhacks_not_controllable") == GLOBAL_ON )
	{
		if (m_bHadControllable)
		{
			m_iManhackHintTimeShown=0;
			CBasePlayer *pPlayer = ToBasePlayer( pOwner );
			if ( pPlayer != NULL && HasNPCManhack())
			{
				UTIL_HudHintText( pPlayer, "#HLSS_Hint_ManhackSend" );
				m_iManhackHintTimeShown++;
			}
		}

		m_bHadControllable = false;
	} 
	else
	{
		if (!m_bHadControllable)
		{
			m_iManhackHintTimeShown=0;
			CBasePlayer *pPlayer = ToBasePlayer( pOwner );
			if ( pPlayer != NULL && HasNPCManhack())
			{
				UTIL_HudHintText( pPlayer, "#HLSS_Hint_ManhackControl" );
				m_iManhackHintTimeShown++;
			}
		}
		m_bHadControllable = true;
	}
#endif

	return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), iActivity, (char*)GetAnimPrefix() );
}

bool CWeapon_Manhack::HasAnyAmmo( void )
{
	//FindVehicleManhack();

	if ( HasNPCManhack() )
	{
		return true;
	}

	CBaseCombatCharacter *pOwner  = GetOwner();
	if (pOwner==NULL) return false;
	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
		return true;

	return false;
}

void CWeapon_Manhack::WeaponSwitch( void )
{
#ifndef CLIENT_DLL
	CBaseCombatCharacter* pOwner = GetOwner();
	if (pOwner == NULL) return;
	pOwner->SwitchToNextBestWeapon(pOwner->GetActiveWeapon());

	if (!HasNPCManhack() && pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		pOwner->ClearActiveWeapon();
	}
#endif // !CLIENT_DLL
}

void CWeapon_Manhack::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType ); //Secondary

}

bool CWeapon_Manhack::Reload( void )
{
	//Deploy();
	WeaponIdle();
	return true;
}
#ifndef CLIENT_DLL
//TERO: This is called when we already have the vehicle manhack (the CP model), it creates the actual NPC manhack
bool CWeapon_Manhack::CreateControllableNPCManhack( CBasePlayer *pPlayer )
{
	/*Vector vecOrigin;
	QAngle vecAngles;

	int turretSpawnAttachment = LookupAttachment( "manhackSpawn" );
	GetAttachment( turretSpawnAttachment, vecOrigin, vecAngles );*/

	Vector	vForward, vRight, vUp, vThrowPos;
	
	pPlayer->EyeVectors( &vForward, &vRight, &vUp );

	vThrowPos = pPlayer->EyePosition();

	vThrowPos += vForward * 20.0f;

	CLaz_Player* pLaz = ToLazuulPlayer(pPlayer);
	if (!pLaz)
		return false;

	CNPC_Manhack* pManhack = pLaz->CreateManhack(vThrowPos, pPlayer->EyeAngles());
	if (!pManhack)
		return false;

	pManhack->AddSpawnFlags(SF_MANHACK_PACKED_UP);
	pManhack->Spawn();
	pManhack->Activate();

	return true;
}

void CWeapon_Manhack::DriveControllableManhack()
{
	CLaz_Player* pPlayer = ToLazuulPlayer(GetOwner());

	if (pPlayer != NULL)
	{
		CPropVehicleManhack* pVehicle = VehicleManhack_Create(pPlayer, pPlayer->GetCurrentManhack());
		pVehicle->ForcePlayerIn(pPlayer);
	}
}
#endif
bool CWeapon_Manhack::HasNPCManhack()
{
	return NumberOfManhacks() > 0;
}

bool CWeapon_Manhack::HasFreeSlot()
{
	CLaz_Player* pPlayer = ToLazuulPlayer(GetOwner());
	if (pPlayer != NULL)
	{
		if (pPlayer->GetManhackCount() >= NUMBER_OF_CONTROLLABLE_MANHACKS)
		{
			return false;
		}

		return true;
	}

	return false;
}


int CWeapon_Manhack::NumberOfManhacks()
{
	CLaz_Player* pPlayer = ToLazuulPlayer(GetOwner());
	if (pPlayer !=NULL)
	{
		return pPlayer->GetManhackCount();
	}	
	return 0;
}

