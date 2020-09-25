//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose:		Player for .
//
//===========================================================================//

#include "cbase.h"
#include "vcollide_parse.h"
#include "c_portal_player.h"
#include "view.h"
#include "c_basetempentity.h"
#include "takedamageinfo.h"
#include "in_buttons.h"
#include "iviewrender_beams.h"
#include "r_efx.h"
#include "dlight.h"
#include "portalrender.h"
#include "toolframework/itoolframework.h"
#include "toolframework_client.h"
#include "tier1/keyvalues.h"
#include "ScreenSpaceEffects.h"
#include "portal_shareddefs.h"
#include "ivieweffects.h"		// for screenshake
#include "prop_portal_shared.h"


// Don't alias here
#if defined( CPortal_Player )
#undef CPortal_Player
#endif


#define REORIENTATION_RATE 120.0f
#define REORIENTATION_ACCELERATION_RATE 400.0f

#define ENABLE_PORTAL_EYE_INTERPOLATION_CODE



ConVar cl_reorient_in_air("cl_reorient_in_air", "1", FCVAR_ARCHIVE, "Allows the player to only reorient from being upside down while in the air." );


IMPLEMENT_CLIENTCLASS_DT(C_Portal_Player, DT_Portal_Player, CPortal_Player)
RecvPropBool( RECVINFO( m_bHeldObjectOnOppositeSideOfPortal ) ),
RecvPropEHandle( RECVINFO( m_pHeldObjectPortal ) ),
RecvPropBool( RECVINFO( m_bPitchReorientation ) ),
RecvPropEHandle( RECVINFO( m_hPortalEnvironment ) ),
RecvPropEHandle( RECVINFO( m_hSurroundingLiquidPortal ) ),
RecvPropBool( RECVINFO ( m_bSuppressCrosshair ) ),
END_RECV_TABLE()


BEGIN_PREDICTION_DATA( C_Portal_Player )
END_PREDICTION_DATA()

#define	_WALK_SPEED 150
#define	_NORM_SPEED 190
#define	_SPRINT_SPEED 320

static ConVar cl_playermodel( "cl_playermodel", "none", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Default Player Model");

extern bool g_bUpsideDown;

//EHANDLE g_eKillTarget1;
//EHANDLE g_eKillTarget2;

void SpawnBlood (Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage);

C_Portal_Player::C_Portal_Player()
{
	m_bHeldObjectOnOppositeSideOfPortal = false;
	m_pHeldObjectPortal = 0;

	m_bPitchReorientation = false;
	m_fReorientationRate = 0.0f;
}

C_Portal_Player::~C_Portal_Player( void )
{
	
}

void C_Portal_Player::ClientThink( void )
{
	BaseClass::ClientThink();

	FixTeleportationRoll();
}

void C_Portal_Player::FixTeleportationRoll( void )
{
	if( IsInAVehicle() ) //HL2 compatibility fix. do absolutely nothing to the view in vehicles
		return;

	if( !IsLocalPlayer() )
		return;

	// Normalize roll from odd portal transitions
	QAngle vAbsAngles = EyeAngles();


	Vector vCurrentForward, vCurrentRight, vCurrentUp;
	AngleVectors( vAbsAngles, &vCurrentForward, &vCurrentRight, &vCurrentUp );

	if ( vAbsAngles[ROLL] == 0.0f )
	{
		m_fReorientationRate = 0.0f;
		g_bUpsideDown = ( vCurrentUp.z < 0.0f );
		return;
	}

	bool bForcePitchReorient = ( vAbsAngles[ROLL] > 175.0f && vCurrentForward.z > 0.99f );
	bool bOnGround = ( GetGroundEntity() != NULL );

	if ( bForcePitchReorient )
	{
		m_fReorientationRate = REORIENTATION_RATE * ( ( bOnGround ) ? ( 2.0f ) : ( 1.0f ) );
	}
	else
	{
		// Don't reorient in air if they don't want to
		if ( !cl_reorient_in_air.GetBool() && !bOnGround )
		{
			g_bUpsideDown = ( vCurrentUp.z < 0.0f );
			return;
		}
	}

	if ( vCurrentUp.z < 0.75f )
	{
		m_fReorientationRate += gpGlobals->frametime * REORIENTATION_ACCELERATION_RATE;

		// Upright faster if on the ground
		float fMaxReorientationRate = REORIENTATION_RATE * ( ( bOnGround ) ? ( 2.0f ) : ( 1.0f ) );
		if ( m_fReorientationRate > fMaxReorientationRate )
			m_fReorientationRate = fMaxReorientationRate;
	}
	else
	{
		if ( m_fReorientationRate > REORIENTATION_RATE * 0.5f )
		{
			m_fReorientationRate -= gpGlobals->frametime * REORIENTATION_ACCELERATION_RATE;
			if ( m_fReorientationRate < REORIENTATION_RATE * 0.5f )
				m_fReorientationRate = REORIENTATION_RATE * 0.5f;
		}
		else if ( m_fReorientationRate < REORIENTATION_RATE * 0.5f )
		{
			m_fReorientationRate += gpGlobals->frametime * REORIENTATION_ACCELERATION_RATE;
			if ( m_fReorientationRate > REORIENTATION_RATE * 0.5f )
				m_fReorientationRate = REORIENTATION_RATE * 0.5f;
		}
	}

	if ( !m_bPitchReorientation && !bForcePitchReorient )
	{
		// Randomize which way we roll if we're completely upside down
		if ( vAbsAngles[ROLL] == 180.0f && RandomInt( 0, 1 ) == 1 )
		{
			vAbsAngles[ROLL] = -180.0f;
		}

		if ( vAbsAngles[ROLL] < 0.0f )
		{
			vAbsAngles[ROLL] += gpGlobals->frametime * m_fReorientationRate;
			if ( vAbsAngles[ROLL] > 0.0f )
				vAbsAngles[ROLL] = 0.0f;
			engine->SetViewAngles( vAbsAngles );
		}
		else if ( vAbsAngles[ROLL] > 0.0f )
		{
			vAbsAngles[ROLL] -= gpGlobals->frametime * m_fReorientationRate;
			if ( vAbsAngles[ROLL] < 0.0f )
				vAbsAngles[ROLL] = 0.0f;
			engine->SetViewAngles( vAbsAngles );
			m_angEyeAngles = vAbsAngles;
			m_iv_angEyeAngles.Reset();
		}
	}
	else
	{
		if ( vAbsAngles[ROLL] != 0.0f )
		{
			if ( vCurrentUp.z < 0.2f )
			{
				float fDegrees = gpGlobals->frametime * m_fReorientationRate;
				if ( vCurrentForward.z > 0.0f )
				{
					fDegrees = -fDegrees;
				}

				// Rotate around the right axis
				VMatrix mAxisAngleRot = SetupMatrixAxisRot( vCurrentRight, fDegrees );

				vCurrentUp = mAxisAngleRot.VMul3x3( vCurrentUp );
				vCurrentForward = mAxisAngleRot.VMul3x3( vCurrentForward );

				VectorAngles( vCurrentForward, vCurrentUp, vAbsAngles );

				engine->SetViewAngles( vAbsAngles );
				m_angEyeAngles = vAbsAngles;
				m_iv_angEyeAngles.Reset();
			}
			else
			{
				if ( vAbsAngles[ROLL] < 0.0f )
				{
					vAbsAngles[ROLL] += gpGlobals->frametime * m_fReorientationRate;
					if ( vAbsAngles[ROLL] > 0.0f )
						vAbsAngles[ROLL] = 0.0f;
					engine->SetViewAngles( vAbsAngles );
					m_angEyeAngles = vAbsAngles;
					m_iv_angEyeAngles.Reset();
				}
				else if ( vAbsAngles[ROLL] > 0.0f )
				{
					vAbsAngles[ROLL] -= gpGlobals->frametime * m_fReorientationRate;
					if ( vAbsAngles[ROLL] < 0.0f )
						vAbsAngles[ROLL] = 0.0f;
					engine->SetViewAngles( vAbsAngles );
					m_angEyeAngles = vAbsAngles;
					m_iv_angEyeAngles.Reset();
				}
			}
		}
	}

	// Keep track of if we're upside down for look control
	vAbsAngles = EyeAngles();
	AngleVectors( vAbsAngles, NULL, NULL, &vCurrentUp );

	if ( bForcePitchReorient )
		g_bUpsideDown = ( vCurrentUp.z < 0.0f );
	else
		g_bUpsideDown = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int C_Portal_Player::DrawModel( int flags )
{
	if ( !m_bReadyToDraw )
		return 0;

	if( IsLocalPlayer() && ShouldDoPortalRenderCulling())
	{
		if ( !C_BasePlayer::ShouldDrawLocalPlayer() )
		{
			if ( !g_pPortalRender->IsRenderingPortal() )
				return 0;

			if( (g_pPortalRender->GetViewRecursionLevel() == 1) && (m_iForceNoDrawInPortalSurface != -1) ) //CPortalRender::s_iRenderingPortalView )
				return 0;
		}
	}

	return BaseClass::DrawModel(flags);
}

void C_Portal_Player::DoImpactEffect( trace_t &tr, int nDamageType )
{
	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->DoImpactEffect( tr, nDamageType );
		return;
	}

	BaseClass::DoImpactEffect( tr, nDamageType );
}

bool C_Portal_Player::ShouldDraw(void)
{
	if (!IsAlive())
		return false;

	//return true;

	if (GetTeamNumber() == TEAM_SPECTATOR)
		return false;

	if (GetObserverMode() != OBS_MODE_NONE)
		return false;

	/*if( IsLocalPlayer() && IsRagdoll() )
		return true;*/

	if (IsRagdoll())
		return false;

	// Some rendermodes prevent rendering
	if (GetRenderMode() == kRenderNone)
		return false;

	return !IsEffectActive(EF_NODRAW);
}

void C_Portal_Player::PlayerPortalled( C_Prop_Portal *pEnteredPortal )
{
	if( pEnteredPortal )
	{
		m_bPortalledMessagePending = true;
		m_PendingPortalMatrix = pEnteredPortal->MatrixThisToLinked();

		if( IsLocalPlayer() )
			g_pPortalRender->EnteredPortal( pEnteredPortal );
	}
}

void C_Portal_Player::OnPreDataChanged( DataUpdateType_t type )
{
	Assert( m_pPortalEnvironment_LastCalcView == m_hPortalEnvironment.Get() );
	PreDataChanged_Backup.m_hPortalEnvironment = m_hPortalEnvironment;
	PreDataChanged_Backup.m_hSurroundingLiquidPortal = m_hSurroundingLiquidPortal;
	PreDataChanged_Backup.m_qEyeAngles = m_iv_angEyeAngles.GetCurrent();

	BaseClass::OnPreDataChanged( type );
}

void C_Portal_Player::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if( m_hSurroundingLiquidPortal != PreDataChanged_Backup.m_hSurroundingLiquidPortal )
	{
		CLiquidPortal_InnerLiquidEffect *pLiquidEffect = (CLiquidPortal_InnerLiquidEffect *)g_pScreenSpaceEffects->GetScreenSpaceEffect( "LiquidPortal_InnerLiquid" );
		if( pLiquidEffect )
		{
			C_Func_LiquidPortal *pSurroundingPortal = m_hSurroundingLiquidPortal.Get();
			if( pSurroundingPortal != NULL )
			{
				C_Func_LiquidPortal *pOldSurroundingPortal = PreDataChanged_Backup.m_hSurroundingLiquidPortal.Get();
				if( pOldSurroundingPortal != pSurroundingPortal->m_hLinkedPortal.Get() )
				{
					pLiquidEffect->m_pImmersionPortal = pSurroundingPortal;
					pLiquidEffect->m_bFadeBackToReality = false;
				}
				else
				{
					pLiquidEffect->m_bFadeBackToReality = true;
					pLiquidEffect->m_fFadeBackTimeLeft = pLiquidEffect->s_fFadeBackEffectTime;
				}
			}
			else
			{
				pLiquidEffect->m_pImmersionPortal = NULL;
				pLiquidEffect->m_bFadeBackToReality = false;
			}
		}
	}

	DetectAndHandlePortalTeleportation();

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	UpdateVisibility();
}

//CalcView() gets called between OnPreDataChanged() and OnDataChanged(), and these changes need to be known about in both before CalcView() gets called, and if CalcView() doesn't get called
bool C_Portal_Player::DetectAndHandlePortalTeleportation( void )
{
	if( m_bPortalledMessagePending )
	{
		m_bPortalledMessagePending = false;

		//C_Prop_Portal *pOldPortal = PreDataChanged_Backup.m_hPortalEnvironment.Get();
		//Assert( pOldPortal );
		//if( pOldPortal )
		{
			Vector ptNewPosition = GetNetworkOrigin();

			UTIL_Portal_PointTransform( m_PendingPortalMatrix, PortalEyeInterpolation.m_vEyePosition_Interpolated, PortalEyeInterpolation.m_vEyePosition_Interpolated );
			UTIL_Portal_PointTransform( m_PendingPortalMatrix, PortalEyeInterpolation.m_vEyePosition_Uninterpolated, PortalEyeInterpolation.m_vEyePosition_Uninterpolated );

			PortalEyeInterpolation.m_bEyePositionIsInterpolating = true;

			UTIL_Portal_AngleTransform( m_PendingPortalMatrix, m_qEyeAngles_LastCalcView, m_angEyeAngles );
			m_angEyeAngles.x = AngleNormalize( m_angEyeAngles.x );
			m_angEyeAngles.y = AngleNormalize( m_angEyeAngles.y );
			m_angEyeAngles.z = AngleNormalize( m_angEyeAngles.z );
			m_iv_angEyeAngles.Reset(); //copies from m_angEyeAngles

			if( engine->IsPlayingDemo() )
			{
				pl.v_angle = m_angEyeAngles;
				engine->SetViewAngles( pl.v_angle );
			}

			engine->ResetDemoInterpolation();
			if( IsLocalPlayer() )
			{
				//DevMsg( "FPT: %.2f %.2f %.2f\n", m_angEyeAngles.x, m_angEyeAngles.y, m_angEyeAngles.z );
				SetLocalAngles( m_angEyeAngles );
			}

			if (GetAnimState())
				GetAnimState()->m_bForceAimYaw = true;

			// Reorient last facing direction to fix pops in view model lag
			for ( int i = 0; i < MAX_VIEWMODELS; i++ )
			{
				CBaseViewModel *vm = GetViewModel( i );
				if ( !vm )
					continue;

				UTIL_Portal_VectorTransform( m_PendingPortalMatrix, vm->m_vecLastFacing, vm->m_vecLastFacing );
			}
		}
		m_bPortalledMessagePending = false;
	}

	return false;
}

/*bool C_Portal_Player::ShouldInterpolate( void )
{
if( !IsInterpolationEnabled() )
return false;

return BaseClass::ShouldInterpolate();
}*/


void C_Portal_Player::ItemPreFrame( void )
{
	if ( GetFlags() & FL_FROZEN )
		return;

	// Disallow shooting while zooming
	if ( m_nButtons & IN_ZOOM )
	{
		//FIXME: Held weapons like the grenade get sad when this happens
		m_nButtons &= ~(IN_ATTACK|IN_ATTACK2);
	}

	BaseClass::ItemPreFrame();

}

void C_Portal_Player::ItemPostFrame( void )
{
	if ( GetFlags() & FL_FROZEN )
		return;

	BaseClass::ItemPostFrame();
}

void C_Portal_Player::UpdatePortalEyeInterpolation( void )
{
#ifdef ENABLE_PORTAL_EYE_INTERPOLATION_CODE
	//PortalEyeInterpolation.m_bEyePositionIsInterpolating = false;
	if( PortalEyeInterpolation.m_bUpdatePosition_FreeMove )
	{
		PortalEyeInterpolation.m_bUpdatePosition_FreeMove = false;

		C_Prop_Portal *pOldPortal = PreDataChanged_Backup.m_hPortalEnvironment.Get();
		if( pOldPortal )
		{
			UTIL_Portal_PointTransform( pOldPortal->MatrixThisToLinked(), PortalEyeInterpolation.m_vEyePosition_Interpolated, PortalEyeInterpolation.m_vEyePosition_Interpolated );
			//PortalEyeInterpolation.m_vEyePosition_Interpolated = pOldPortal->m_matrixThisToLinked * PortalEyeInterpolation.m_vEyePosition_Interpolated;

			//Vector vForward;
			//m_hPortalEnvironment.Get()->GetVectors( &vForward, NULL, NULL );

			PortalEyeInterpolation.m_vEyePosition_Interpolated = EyeFootPosition();

			PortalEyeInterpolation.m_bEyePositionIsInterpolating = true;
		}
	}

	if( IsInAVehicle() )
		PortalEyeInterpolation.m_bEyePositionIsInterpolating = false;

	if( !PortalEyeInterpolation.m_bEyePositionIsInterpolating )
	{
		PortalEyeInterpolation.m_vEyePosition_Uninterpolated = EyeFootPosition();
		PortalEyeInterpolation.m_vEyePosition_Interpolated = PortalEyeInterpolation.m_vEyePosition_Uninterpolated;
		return;
	}

	Vector vThisFrameUninterpolatedPosition = EyeFootPosition();

	//find offset between this and last frame's uninterpolated movement, and apply this as freebie movement to the interpolated position
	PortalEyeInterpolation.m_vEyePosition_Interpolated += (vThisFrameUninterpolatedPosition - PortalEyeInterpolation.m_vEyePosition_Uninterpolated);
	PortalEyeInterpolation.m_vEyePosition_Uninterpolated = vThisFrameUninterpolatedPosition;

	Vector vDiff = vThisFrameUninterpolatedPosition - PortalEyeInterpolation.m_vEyePosition_Interpolated;
	float fLength = vDiff.Length();
	float fFollowSpeed = gpGlobals->frametime * 100.0f;
	const float fMaxDiff = 150.0f;
	if( fLength > fMaxDiff )
	{
		//camera lagging too far behind, give it a speed boost to bring it within maximum range
		fFollowSpeed = fLength - fMaxDiff;
	}
	else if( fLength < fFollowSpeed )
	{
		//final move
		PortalEyeInterpolation.m_bEyePositionIsInterpolating = false;
		PortalEyeInterpolation.m_vEyePosition_Interpolated = vThisFrameUninterpolatedPosition;
		return;
	}

	if ( fLength > 0.001f )
	{
		vDiff *= (fFollowSpeed/fLength);
		PortalEyeInterpolation.m_vEyePosition_Interpolated += vDiff;
	}
	else
	{
		PortalEyeInterpolation.m_vEyePosition_Interpolated = vThisFrameUninterpolatedPosition;
	}



#else
	PortalEyeInterpolation.m_vEyePosition_Interpolated = BaseClass::EyePosition();
#endif
}

Vector C_Portal_Player::EyePosition()
{
	return PortalEyeInterpolation.m_vEyePosition_Interpolated;
}

Vector C_Portal_Player::EyeFootPosition( const QAngle &qEyeAngles )
{
#if 0
	static int iPrintCounter = 0;
	++iPrintCounter;
	if( iPrintCounter == 50 )
	{
		QAngle vAbsAngles = qEyeAngles;
		DevMsg( "Eye Angles: %f %f %f\n", vAbsAngles.x, vAbsAngles.y, vAbsAngles.z );
		iPrintCounter = 0;
	}
#endif

	//interpolate between feet and normal eye position based on view roll (gets us wall/ceiling & ceiling/ceiling teleportations without an eye position pop)
	float fFootInterp = fabsf(qEyeAngles[ROLL]) * ((1.0f/180.0f) * 0.75f); //0 when facing straight up, 0.75 when facing straight down
	return (BaseClass::EyePosition() - (fFootInterp * m_vecViewOffset)); //TODO: Find a good Up vector for this rolled player and interpolate along actual eye/foot axis
}

void C_Portal_Player::CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov )
{
	DetectAndHandlePortalTeleportation();
	//if( DetectAndHandlePortalTeleportation() )
	//	DevMsg( "Teleported within OnDataChanged\n" );

	m_iForceNoDrawInPortalSurface = -1;
	bool bEyeTransform_Backup = m_bEyePositionIsTransformedByPortal;
	m_bEyePositionIsTransformedByPortal = false; //assume it's not transformed until it provably is
	UpdatePortalEyeInterpolation();

	QAngle qEyeAngleBackup = EyeAngles();
	Vector ptEyePositionBackup = EyePosition();
	C_Prop_Portal *pPortalBackup = m_hPortalEnvironment.Get();

	if ( m_lifeState != LIFE_ALIVE )
	{
		BaseClass::CalcView(eyeOrigin, eyeAngles, zNear, zFar, fov);
	}
	else
	{
		IClientVehicle *pVehicle;
		pVehicle = GetVehicle();

		if ( !pVehicle )
		{
			if ( IsObserver() )
			{
				CalcObserverView( eyeOrigin, eyeAngles, fov );
			}
			else
			{
				CalcPlayerView( eyeOrigin, eyeAngles, fov );
				if( m_hPortalEnvironment.Get() != NULL )
				{
					//time for hax
					m_bEyePositionIsTransformedByPortal = bEyeTransform_Backup;
					CalcPortalView( eyeOrigin, eyeAngles );
				}
			}
		}
		else
		{
			CalcVehicleView( pVehicle, eyeOrigin, eyeAngles, zNear, zFar, fov );
		}
	}

	m_qEyeAngles_LastCalcView = qEyeAngleBackup;
	m_ptEyePosition_LastCalcView = ptEyePositionBackup;
	m_pPortalEnvironment_LastCalcView = pPortalBackup;
}

void C_Portal_Player::SetLocalViewAngles( const QAngle &viewAngles )
{
	// Nothing
	if ( engine->IsPlayingDemo() )
		return;
	BaseClass::SetLocalViewAngles( viewAngles );
}

void C_Portal_Player::SetViewAngles( const QAngle& ang )
{
	BaseClass::SetViewAngles( ang );

	if ( engine->IsPlayingDemo() )
	{
		pl.v_angle = ang;
	}
}

void C_Portal_Player::CalcPortalView( Vector &eyeOrigin, QAngle &eyeAngles )
{
	//although we already ran CalcPlayerView which already did these copies, they also fudge these numbers in ways we don't like, so recopy
	VectorCopy( EyePosition(), eyeOrigin );
	VectorCopy( EyeAngles(), eyeAngles );

	//Re-apply the screenshake (we just stomped it)
	vieweffects->ApplyShake( eyeOrigin, eyeAngles, 1.0 );

	C_Prop_Portal *pPortal = m_hPortalEnvironment.Get();
	Assert( pPortal );

	C_Prop_Portal *pRemotePortal = pPortal->m_hLinkedPortal;
	if( !pRemotePortal )
	{
		return; //no hacks possible/necessary
	}

	Vector ptPortalCenter;
	Vector vPortalForward;

	ptPortalCenter = pPortal->GetNetworkOrigin();
	pPortal->GetVectors( &vPortalForward, NULL, NULL );
	float fPortalPlaneDist = vPortalForward.Dot( ptPortalCenter );

	bool bOverrideSpecialEffects = false; //sometimes to get the best effect we need to kill other effects that are simply for cleanliness

	float fEyeDist = vPortalForward.Dot( eyeOrigin ) - fPortalPlaneDist;
	bool bTransformEye = false;
	if( fEyeDist < 0.0f ) //eye behind portal
	{
		if( pPortal->m_PortalSimulator.EntityIsInPortalHole( this ) ) //player standing in portal
		{
			bTransformEye = true;
		}
		else if( vPortalForward.z < -0.01f ) //there's a weird case where the player is ducking below a ceiling portal. As they unduck their eye moves beyond the portal before the code detects that they're in the portal hole.
		{
			Vector ptPlayerOrigin = GetAbsOrigin();
			float fOriginDist = vPortalForward.Dot( ptPlayerOrigin ) - fPortalPlaneDist;

			if( fOriginDist > 0.0f )
			{
				float fInvTotalDist = 1.0f / (fOriginDist - fEyeDist); //fEyeDist is negative
				Vector ptPlaneIntersection = (eyeOrigin * fOriginDist * fInvTotalDist) - (ptPlayerOrigin * fEyeDist * fInvTotalDist);
				Assert( fabsf( vPortalForward.Dot( ptPlaneIntersection ) - fPortalPlaneDist ) < 0.01f );

				Vector vIntersectionTest = ptPlaneIntersection - ptPortalCenter;

				Vector vPortalRight, vPortalUp;
				pPortal->GetVectors( NULL, &vPortalRight, &vPortalUp );

				if( (vIntersectionTest.Dot( vPortalRight ) <= PORTAL_HALF_WIDTH) &&
					(vIntersectionTest.Dot( vPortalUp ) <= PORTAL_HALF_HEIGHT) )
				{
					bTransformEye = true;
				}
			}
		}
	}

	if( bTransformEye )
	{
		m_bEyePositionIsTransformedByPortal = true;

		//DevMsg( 2, "transforming portal view from <%f %f %f> <%f %f %f>\n", eyeOrigin.x, eyeOrigin.y, eyeOrigin.z, eyeAngles.x, eyeAngles.y, eyeAngles.z );

		VMatrix matThisToLinked = pPortal->MatrixThisToLinked();
		UTIL_Portal_PointTransform( matThisToLinked, eyeOrigin, eyeOrigin );
		UTIL_Portal_AngleTransform( matThisToLinked, eyeAngles, eyeAngles );

		//DevMsg( 2, "transforming portal view to   <%f %f %f> <%f %f %f>\n", eyeOrigin.x, eyeOrigin.y, eyeOrigin.z, eyeAngles.x, eyeAngles.y, eyeAngles.z );

		if ( IsToolRecording() )
		{
			static EntityTeleportedRecordingState_t state;

			KeyValues *msg = new KeyValues( "entity_teleported" );
			msg->SetPtr( "state", &state );
			state.m_bTeleported = false;
			state.m_bViewOverride = true;
			state.m_vecTo = eyeOrigin;
			state.m_qaTo = eyeAngles;
			MatrixInvert( matThisToLinked.As3x4(), state.m_teleportMatrix );

			// Post a message back to all IToolSystems
			Assert( (int)GetToolHandle() != 0 );
			ToolFramework_PostToolMessage( GetToolHandle(), msg );

			msg->deleteThis();
		}

		bOverrideSpecialEffects = true;
	}
	else
	{
		m_bEyePositionIsTransformedByPortal = false;
	}

	if( bOverrideSpecialEffects )
	{
		m_iForceNoDrawInPortalSurface = ((pRemotePortal->m_bIsPortal2)?(2):(1));
		pRemotePortal->m_fStaticAmount = 0.0f;
	}
}

extern float g_fMaxViewModelLag;
void C_Portal_Player::CalcViewModelView( const Vector& eyeOrigin, const QAngle& eyeAngles)
{
	// HACK: Manually adjusting the eye position that view model looking up and down are similar
	// (solves view model "pop" on floor to floor transitions)
	Vector vInterpEyeOrigin = eyeOrigin;

	Vector vForward;
	Vector vRight;
	Vector vUp;
	AngleVectors( eyeAngles, &vForward, &vRight, &vUp );

	if ( gpGlobals->maxClients == 1 && vForward.z < 0.0f )
	{
		CBaseCombatWeapon* pWeapon = GetActiveWeapon();
		float exp = 0;

		if (pWeapon != NULL)
		{

			//get delta time for interpolation
			float delta = (gpGlobals->curtime - pWeapon->m_flIronsightedTime) * 2.5f; //modify this value to adjust how fast the interpolation is
			exp = (pWeapon->IsIronsighted()) ?
				(delta > 1.0f) ? 1.0f : delta : //normal blending
				(delta > 1.0f) ? 0.0f : 1.0f - delta; //reverse interpolation

			if (exp <= 0.001f) //fully not ironsighted; save performance
				exp = 0;
		}

		float fT = vForward.z * vForward.z * (1.f - exp);
		vInterpEyeOrigin += vRight * ( fT * 4.7f ) + vForward * ( fT * 5.0f ) + vUp * ( fT * 4.0f );
	}

	if ( UTIL_IntersectEntityExtentsWithPortal( this ) )
		g_fMaxViewModelLag = 0.0f;
	else
		g_fMaxViewModelLag = 1.5f;

	for ( int i = 0; i < MAX_VIEWMODELS; i++ )
	{
		CBaseViewModel *vm = GetViewModel( i );
		if ( !vm )
			continue;

		vm->CalcViewModelView( this, vInterpEyeOrigin, eyeAngles );
	}
}

bool LocalPlayerIsCloseToPortal( void )
{
	return C_Portal_Player::GetLocalPlayer()->IsCloseToPortal();
}

