//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_AI_BASENPC_H
#define C_AI_BASENPC_H
#ifdef _WIN32
#pragma once
#endif


#include "c_basecombatcharacter.h"

// NOTE: Moved all controller code into c_basestudiomodel
class C_AI_BaseNPC : public C_BaseCombatCharacter, public IHasAttributes
{
	DECLARE_CLASS( C_AI_BaseNPC, C_BaseCombatCharacter );

public:
	DECLARE_CLIENTCLASS();

	C_AI_BaseNPC();
	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const;
	virtual	bool		ShouldCollide(int collisionGroup, int contentsMask) const;
	virtual bool			IsNPC( void ) { return true; }
	bool					IsMoving( void ){ return m_bIsMoving; }
	bool					ShouldAvoidObstacle( void ){ return m_bPerformAvoidance; }
	virtual bool			AddRagdollToFadeQueue( void ) { return m_bFadeCorpse; }

	virtual bool			GetRagdollInitBoneArrays( matrix3x4_t *pDeltaBones0, matrix3x4_t *pDeltaBones1, matrix3x4_t *pCurrentBones, float boneDt ) OVERRIDE;

	int						GetDeathPose( void ) { return m_iDeathPose; }

	virtual Vector			GetObserverViewOffset(void);

	bool					ShouldModifyPlayerSpeed( void ) { return m_bSpeedModActive;	}
	int						GetSpeedModifyRadius( void ) { return m_iSpeedModRadius; }
	int						GetSpeedModifySpeed( void ) { return m_iSpeedModSpeed;	}

	void					ClientThink( void );
	void					OnDataChanged( DataUpdateType_t type );
	bool					ImportantRagdoll( void ) { return m_bImportanRagdoll;	}

	virtual int		GetHealth() const { return m_iHealth; }
	virtual int		GetMaxHealth() const { return m_iMaxHealth; }

	virtual CAttributeManager* GetAttributeManager() { return &m_AttributeManager; }
	virtual CAttributeContainer* GetAttributeContainer() { return NULL; }
	virtual CBaseEntity* GetAttributeOwner() { return NULL; }
	virtual void ReapplyProvision(void) { /*Do nothing*/ };

#ifdef HL2_LAZUL
	virtual int GetStepSound(bool bLeft, bool bRunning)
	{
		short iIndex = 0;
		if (!bLeft)
			iIndex += 1;
		if (bRunning)
			iIndex += 2;

		Assert(iIndex < NUM_NPC_STEP_SOUNDS);
		return m_iStepSounds[iIndex];
	}
#endif // HL2_LAZUL


private:
	C_AI_BaseNPC( const C_AI_BaseNPC & ); // not defined, not accessible
	float m_flTimePingEffect;
	int  m_iDeathPose;
	int	 m_iDeathFrame;
	int		m_iMaxHealth;

	int m_iSpeedModRadius;
	int m_iSpeedModSpeed;

	bool m_bPerformAvoidance;
	bool m_bIsMoving;
	bool m_bFadeCorpse;
	bool m_bSpeedModActive;
	bool m_bImportanRagdoll;

	CAttributeManager m_AttributeManager;

#ifdef HL2_LAZUL
	int m_iStepSounds[NUM_NPC_STEP_SOUNDS];
#endif // HL2_LAZUL
};


#endif // C_AI_BASENPC_H
