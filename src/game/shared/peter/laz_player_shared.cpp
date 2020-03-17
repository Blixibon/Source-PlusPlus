//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"

#ifdef CLIENT_DLL
#include "peter/c_laz_player.h"
#include "prediction.h"
#define CRecipientFilter C_RecipientFilter
#else
#include "peter/laz_player.h"
#endif

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "collisionutils.h"
#include "bone_setup.h"

#define PLAYER_HULL_REDUCTION	0.70

#define HEAD_RAND_BOUNDS 5.0f
#define EYE_RAND_BOUNDS 3.0f

extern ConVar sv_footsteps;

inline bool IsInvalidString(int iString) { return (unsigned short)iString == INVALID_STRING_INDEX || iString < -1; }

enum
{
	CHAN_BODY2 = CHAN_USER_BASE + CHAN_BODY,
};

HL1Foot_t s_pHL1FootSounds[26] =
{
	{ "HL1.Default.StepLeft", "HL1.Default.StepRight" },	// CHAR_TEX_ANTLION
{ "HL1.Flesh.StepLeft", "HL1.Flesh.StepRight" },	// CHAR_TEX_BLOODYFLESH	
{ "HL1.Default.StepLeft", "HL1.Default.StepRight" },	// CHAR_TEX_CONCRETE		
{ "HL1.Dirt.StepLeft", "HL1.Dirt.StepRight" },	// CHAR_TEX_DIRT			
{ "HL1.Default.StepLeft", "HL1.Default.StepRight" },	// CHAR_TEX_EGGSHELL		
{ "HL1.Flesh.StepLeft", "HL1.Flesh.StepRight" },	// CHAR_TEX_FLESH			
{ "HL1.Grate.StepLeft", "HL1.Grate.StepRight" },	// CHAR_TEX_GRATE			
{ "HL1.Flesh.StepLeft", "HL1.Flesh.StepRight" },	// CHAR_TEX_ALIENFLESH		
{ "HL1.Default.StepLeft", "HL1.Default.StepRight" },	// CHAR_TEX_CLIP			
{ "HL1.Default.StepLeft", "HL1.Default.StepRight" },	// CHAR_TEX_UNUSED		
{ "HL1.Default.StepLeft", "HL1.Default.StepRight" },	// CHAR_TEX_UNUSED		
{ "HL1.Default.StepLeft", "HL1.Default.StepRight" },	// CHAR_TEX_PLASTIC		
{ "HL1.Metal.StepLeft", "HL1.Metal.StepRight" },	// CHAR_TEX_METAL			
{ "HL1.Dirt.StepLeft", "HL1.Dirt.StepRight" },	// CHAR_TEX_SAND			
{ "HL1.Dirt.StepLeft", "HL1.Dirt.StepRight" },	// CHAR_TEX_FOLIAGE		
{ "HL1.Default.StepLeft", "HL1.Default.StepRight" },	// CHAR_TEX_COMPUTER		
{ "HL1.Default.StepLeft", "HL1.Default.StepRight" },	// CHAR_TEX_UNUSED		
{ "HL1.Default.StepLeft", "HL1.Default.StepRight" },	// CHAR_TEX_UNUSED		
{ "HL1.Slosh.StepLeft", "HL1.Slosh.StepRight" },	// CHAR_TEX_SLOSH			
{ "HL1.Tile.StepLeft", "HL1.Tile.StepRight" },			// CHAR_TEX_TILE			
{ "HL1.Wade.StepLeft", "HL1.Wade.StepRight" },	// CHAR_TEX_WADE		
{ "HL1.Vent.StepLeft", "HL1.Vent.StepRight" },	// CHAR_TEX_VENT			
{ "HL1.Wood.StepLeft", "HL1.Wood.StepRight" },	// CHAR_TEX_WOOD			
{ "HL1.Default.StepLeft", "HL1.Default.StepRight" },	// CHAR_TEX_UNUSED		
{ "HL1.Tile.StepLeft", "HL1.Tile.StepRight" },	// CHAR_TEX_GLASS			
{ "HL1.Default.StepLeft", "HL1.Default.StepRight" },	// CHAR_TEX_WARPSHIELD		
};

//const char *g_ppszPlayerSoundPrefixNames[PLAYER_SOUNDS_MAX] =
//{
//	"NPC_Citizen",
//	"NPC_CombineS",
//	"NPC_MetroPolice",
//};

//-----------------------------------------------------------------------------
// Purpose: override how single player rays hit the player
//-----------------------------------------------------------------------------
namespace LAZ_Player_Trace
{
	bool LineCircleIntersection(
		const Vector2D &center,
		const float radius,
		const Vector2D &vLinePt,
		const Vector2D &vLineDir,
		float *fIntersection1,
		float *fIntersection2)
	{
		// Line = P + Vt
		// Sphere = r (assume we've translated to origin)
		// (P + Vt)^2 = r^2
		// VVt^2 + 2PVt + (PP - r^2)
		// Solve as quadratic:  (-b  +/-  sqrt(b^2 - 4ac)) / 2a
		// If (b^2 - 4ac) is < 0 there is no solution.
		// If (b^2 - 4ac) is = 0 there is one solution (a case this function doesn't support).
		// If (b^2 - 4ac) is > 0 there are two solutions.
		Vector2D P;
		float a, b, c, sqr, insideSqr;


		// Translate circle to origin.
		P[0] = vLinePt[0] - center[0];
		P[1] = vLinePt[1] - center[1];

		a = vLineDir.Dot(vLineDir);
		b = 2.0f * P.Dot(vLineDir);
		c = P.Dot(P) - (radius * radius);

		insideSqr = b * b - 4 * a*c;
		if (insideSqr <= 0.000001f)
			return false;

		// Ok, two solutions.
		sqr = (float)FastSqrt(insideSqr);

		float denom = 1.0 / (2.0f * a);

		*fIntersection1 = (-b - sqr) * denom;
		*fIntersection2 = (-b + sqr) * denom;

		return true;
	}

	static void Collision_ClearTrace(const Vector &vecRayStart, const Vector &vecRayDelta, CBaseTrace *pTrace)
	{
		pTrace->startpos = vecRayStart;
		pTrace->endpos = vecRayStart;
		pTrace->endpos += vecRayDelta;
		pTrace->startsolid = false;
		pTrace->allsolid = false;
		pTrace->fraction = 1.0f;
		pTrace->contents = 0;
	}


	bool IntersectRayWithAACylinder(const Ray_t &ray,
		const Vector &center, float radius, float height, CBaseTrace *pTrace)
	{
		Assert(ray.m_IsRay);
		Collision_ClearTrace(ray.m_Start, ray.m_Delta, pTrace);

		// First intersect the ray with the top + bottom planes
		float halfHeight = height * 0.5;

		// Handle parallel case
		Vector vStart = ray.m_Start - center;
		Vector vEnd = vStart + ray.m_Delta;

		float flEnterFrac, flLeaveFrac;
		if (FloatMakePositive(ray.m_Delta.z) < 1e-8)
		{
			if ((vStart.z < -halfHeight) || (vStart.z > halfHeight))
			{
				return false; // no hit
			}
			flEnterFrac = 0.0f; flLeaveFrac = 1.0f;
		}
		else
		{
			// Clip the ray to the top and bottom of box
			flEnterFrac = IntersectRayWithAAPlane(vStart, vEnd, 2, 1, halfHeight);
			flLeaveFrac = IntersectRayWithAAPlane(vStart, vEnd, 2, 1, -halfHeight);

			if (flLeaveFrac < flEnterFrac)
			{
				float temp = flLeaveFrac;
				flLeaveFrac = flEnterFrac;
				flEnterFrac = temp;
			}

			if (flLeaveFrac < 0 || flEnterFrac > 1)
			{
				return false;
			}
		}

		// Intersect with circle
		float flCircleEnterFrac, flCircleLeaveFrac;
		if (!LineCircleIntersection(vec3_origin.AsVector2D(), radius,
			vStart.AsVector2D(), ray.m_Delta.AsVector2D(), &flCircleEnterFrac, &flCircleLeaveFrac))
		{
			return false; // no hit
		}

		Assert(flCircleEnterFrac <= flCircleLeaveFrac);
		if (flCircleLeaveFrac < 0 || flCircleEnterFrac > 1)
		{
			return false;
		}

		if (flEnterFrac < flCircleEnterFrac)
			flEnterFrac = flCircleEnterFrac;
		if (flLeaveFrac > flCircleLeaveFrac)
			flLeaveFrac = flCircleLeaveFrac;

		if (flLeaveFrac < flEnterFrac)
			return false;

		VectorMA(ray.m_Start, flEnterFrac, ray.m_Delta, pTrace->endpos);
		pTrace->fraction = flEnterFrac;
		pTrace->contents = CONTENTS_SOLID;

		// Calculate the point on our center line where we're nearest the intersection point
		Vector collisionCenter;
		CalcClosestPointOnLineSegment(pTrace->endpos, center + Vector(0, 0, halfHeight), center - Vector(0, 0, halfHeight), collisionCenter);

		// Our normal is the direction from that center point to the intersection point
		pTrace->plane.normal = pTrace->endpos - collisionCenter;
		VectorNormalize(pTrace->plane.normal);

		return true;
	}
}

bool CLaz_Player::TestHitboxes(const Ray_t &ray, unsigned int fContentsMask, trace_t& tr)
{
	if (g_pGameRules->IsMultiplayer())
	{
		return BaseClass::TestHitboxes(ray, fContentsMask, tr);
	}
	else
	{
		Assert(ray.m_IsRay);

		Vector mins, maxs;

		mins = WorldAlignMins();
		maxs = WorldAlignMaxs();

		if (LAZ_Player_Trace::IntersectRayWithAACylinder(ray, WorldSpaceCenter(), maxs.x * PLAYER_HULL_REDUCTION, maxs.z - mins.z, &tr))
		{
			tr.hitbox = 0;
			CStudioHdr *pStudioHdr = GetModelPtr();
			if (!pStudioHdr)
				return false;

			mstudiohitboxset_t *set = pStudioHdr->pHitboxSet(m_nHitboxSet);
			if (!set || !set->numhitboxes)
				return false;

#ifdef CLIENT_DLL
			CBoneCache *pcache = GetBoneCache(pStudioHdr);
#else
			CBoneCache *pcache = GetBoneCache();
#endif

			matrix3x4_t *hitboxbones[MAXSTUDIOBONES];
			pcache->ReadCachedBonePointers(hitboxbones, pStudioHdr->numbones());

			trace_t basetrace;

			if (TraceToStudio(physprops, ray, pStudioHdr, set, hitboxbones, fContentsMask, GetAbsOrigin(), GetModelScale(), basetrace))
			{
				tr.hitbox = basetrace.hitbox;
				tr.hitgroup = basetrace.hitgroup;
			}

			mstudiobbox_t *pbox = set->pHitbox(tr.hitbox);
			mstudiobone_t *pBone = pStudioHdr->pBone(pbox->bone);
			tr.surface.name = "**studio**";
			tr.surface.flags = SURF_HITBOX;
			tr.surface.surfaceProps = physprops->GetSurfaceIndex(pBone->pszSurfaceProp());
		}

		return true;
	}
}

const char *CLaz_Player::GetPlayerModelSoundPrefix(void)
{
	if (IsInvalidString(m_iPlayerSoundType))
		return NULL;

	return g_pStringTablePlayerFootSteps->GetString(m_iPlayerSoundType);
}

void CLaz_Player::PrecacheFootStepSounds(void)
{
	if (m_iPlayerSoundType == FOOTSTEP_SOUND_HL1)
	{
		for (auto effect : s_pHL1FootSounds)
		{
			PrecacheScriptSound(effect.m_pNameLeft);
			PrecacheScriptSound(effect.m_pNameRight);
		}
	}
	else if (!IsInvalidString(m_iPlayerSoundType))
	{
		char szFootStepName[128];

		Q_snprintf(szFootStepName, sizeof(szFootStepName), "%s.RunFootstepLeft", GetPlayerModelSoundPrefix());
		PrecacheScriptSound(szFootStepName);

		Q_snprintf(szFootStepName, sizeof(szFootStepName), "%s.RunFootstepRight", GetPlayerModelSoundPrefix());
		PrecacheScriptSound(szFootStepName);
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : step - 
//			fvol - 
//			force - force sound to play
//-----------------------------------------------------------------------------
void CLaz_Player::PlayStepSound(const Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force)
{
	if (m_iPlayerSoundType == FOOTSTEP_SOUND_HL1)
	{
		if (gpGlobals->maxClients > 1 && !sv_footsteps.GetFloat())
			return;

#if defined( CLIENT_DLL )
		// during prediction play footstep sounds only once
		if (prediction->InPrediction() && !prediction->IsFirstTimePredicted())
			return;
#endif

		if (!psurface)
			return;

		int nSide = m_Local.m_nStepside;
		const HL1Foot_t& effect = s_pHL1FootSounds[psurface->game.material - 'A'];
		const char * pSoundName = nSide ? effect.m_pNameLeft : effect.m_pNameRight;
		if (!pSoundName)
			return;

		m_Local.m_nStepside = !nSide;

		CSoundParameters params;

		Assert(nSide == 0 || nSide == 1);

		{
			// Give child classes an opportunity to override.
			pSoundName = GetOverrideStepSound(pSoundName);

			if (!CBaseEntity::GetParametersForSound(pSoundName, params, NULL))
				return;
		}

		CRecipientFilter filter;
		filter.AddRecipientsByPAS(vecOrigin);

#ifndef CLIENT_DLL
		// in MP, server removes all players in the vecOrigin's PVS, these players generate the footsteps client side
		if (gpGlobals->maxClients > 1)
		{
			filter.RemoveRecipientsByPVS(vecOrigin);
		}
#endif

		EmitSound_t ep;
		ep.m_nChannel = CHAN_BODY;
		ep.m_pSoundName = params.soundname;
#if defined ( TF_DLL ) || defined ( TF_CLIENT_DLL )
		if (TFGameRules()->IsMannVsMachineMode())
		{
			ep.m_flVolume = params.volume;
		}
		else
		{
			ep.m_flVolume = fvol;
		}
#else
		ep.m_flVolume = fvol;
#endif
		ep.m_SoundLevel = params.soundlevel;
		ep.m_nFlags = 0;
		ep.m_nPitch = params.pitch;
		ep.m_pOrigin = &vecOrigin;

		EmitSound(filter, entindex(), ep);

		// Kyle says: ugggh. This function may as well be called "PerformPileOfDesperateGameSpecificFootstepHacks".
		OnEmitFootstepSound(params, vecOrigin, fvol);
	}
	else
	{
		BaseClass::PlayStepSound(vecOrigin, psurface, fvol, force);

		if (IsInvalidString(m_iPlayerSoundType))
			return;

		if (gpGlobals->maxClients > 1 && !sv_footsteps.GetFloat())
			return;

#if defined( CLIENT_DLL )
		// during prediction play footstep sounds only once
		if (!prediction->IsFirstTimePredicted())
			return;
#endif

		if (GetFlags() & FL_DUCKING)
			return;

		//m_Local.m_nStepside = !m_Local.m_nStepside;

		char szStepSound[128];

		if (!m_Local.m_nStepside)
		{
			Q_snprintf(szStepSound, sizeof(szStepSound), "%s.RunFootstepLeft", GetPlayerModelSoundPrefix());
		}
		else
		{
			Q_snprintf(szStepSound, sizeof(szStepSound), "%s.RunFootstepRight", GetPlayerModelSoundPrefix());
		}

		CSoundParameters params;
		if (GetParametersForSound(szStepSound, params, NULL) == false)
			return;

		CRecipientFilter filter;
		filter.AddRecipientsByPAS(vecOrigin);

#ifndef CLIENT_DLL
		// im MP, server removed all players in origins PVS, these players 
		// generate the footsteps clientside
		if (gpGlobals->maxClients > 1)
			filter.RemoveRecipientsByPVS(vecOrigin);
#endif

		EmitSound_t ep;
		ep.m_nChannel = CHAN_AUTO;
		ep.m_pSoundName = params.soundname;
		ep.m_flVolume = fvol;
		ep.m_SoundLevel = params.soundlevel;
		ep.m_nFlags = 0;
		ep.m_nPitch = params.pitch;
		ep.m_pOrigin = &vecOrigin;

		EmitSound(filter, entindex(), ep);
	}
}

bool CLaz_Player::ShouldCollide(int collisionGroup, int contentsMask) const
{
	if (g_pGameRules->IsMultiplayer())
	{
		if (collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT ||
			/*collisionGroup == COLLISION_GROUP_NPC ||*/
			collisionGroup == HL2COLLISION_GROUP_COMBINE_BALL ||
			collisionGroup == HL2COLLISION_GROUP_COMBINE_BALL_NPC)
		{
			switch (GetTeamNumber())
			{
			case TF_TEAM_RED:
				if (!(contentsMask & CONTENTS_REBELTEAM))
					return false;
				break;

			case TF_TEAM_BLUE:
				if (!(contentsMask & CONTENTS_COMBINETEAM))
					return false;
				break;

			case TF_TEAM_GREEN:
				if (!(contentsMask & CONTENTS_GREENTEAM))
					return false;
				break;

			case TF_TEAM_YELLOW:
				if (!(contentsMask & CONTENTS_YELLOWTEAM))
					return false;
				break;
			}
		}
	}

	return BaseClass::ShouldCollide(collisionGroup, contentsMask);
}

ConVar testclassviewheight( "testclassviewheight", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );
Vector vecTestViewHeight(0,0,0);

//-----------------------------------------------------------------------------
// Purpose: Return class-specific standing eye height
//-----------------------------------------------------------------------------
Vector CLaz_Player::GetPlayerEyeHeight(void)
{
	if (testclassviewheight.GetFloat() > 0)
	{
		vecTestViewHeight.z = testclassviewheight.GetFloat();
		return vecTestViewHeight;
	}

	if (m_flEyeHeightOverride > 0)
		return (Vector(0, 0, m_flEyeHeightOverride) * GetModelScale());

	return VEC_VIEW_SCALED(this);
}