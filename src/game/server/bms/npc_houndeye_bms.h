//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef NPC_HOUNDEYE_H
#define NPC_HOUNDEYE_H
#pragma once


#include	"ai_basenpc.h"

//#include	"energy_wave.h"

class CNPC_BMSHoundeye : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_BMSHoundeye, CAI_BaseNPC );
	DECLARE_SERVERCLASS();
public:
	void			Spawn( void );
	void			Precache( void );
	Class_T			Classify ( void );
	void			HandleAnimEvent( animevent_t *pEvent );
	float			MaxYawSpeed ( void );
	void			WarmUpSound ( void );
	void			AlertSound( void );
	void			DeathSound( const CTakeDamageInfo &info );
	void			WarnSound( void );
	void			PainSound( const CTakeDamageInfo &info );
	void			IdleSound( void );
	void			StartTask( const Task_t *pTask );
	void			RunTask( const Task_t *pTask );
	int				GetSoundInterests( void );
	void			SonicAttack( void );
	void			PrescheduleThink( void );
	void			WriteBeamColor ( void );
	int				RangeAttack1Conditions ( float flDot, float flDist );
	bool			FCanActiveIdle ( void );
	virtual int		TranslateSchedule( int scheduleType );
	Activity		NPC_TranslateActivity( Activity eNewActivity );
	virtual int		SelectSchedule( void );
	bool			HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);
	void			NPCThink(void);
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void			Event_Killed( const CTakeDamageInfo &info );
	bool			IsAnyoneInSquadAttacking( void );
	void			SpeakSentence( int sentenceType );
	float			GetChargeTime(int iSequence);

	float			m_flNextSecondaryAttack;
	bool			m_bLoopClockwise;

	//CEnergyWave*	m_pEnergyWave;
	CNetworkVar(float, m_flEndEnergyWaveTime);
	CNetworkVar(float, m_flWaveChargeTime);

	bool			m_fAsleep;// some houndeyes sleep in idle mode if this is set, the houndeye is lying down
	bool			m_fDontBlink;// don't try to open/close eye if this bit is set!

	DEFINE_CUSTOM_AI;
	
	DECLARE_DATADESC();

private:
	int				m_waveRed;
	int				m_waveGreen;
	int				m_waveBlue;
};


#endif // NPC_HOUNDEYE_H
