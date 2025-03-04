//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The worldspawn entity. This spawns first when each level begins.
//
// $NoKeywords: $
//=============================================================================//

#ifndef WORLD_H
#define WORLD_H
#ifdef _WIN32
#pragma once
#endif

#include "bitvec.h"

enum MapVersion_e
{
	MV_EXTERNAL_MAP = 0,

	MV_CURRENT_VERSION
};

enum WorldServerBools
{
	WORLD_DISPLAY_TITLE = 0,
	WORLD_IS_EPISODIC,
	WORLD_EXPECTS_PORTALS,
	WORLD_BURING_DLIGHTS,

	NUM_WORLD_BOOLS
};

class CWorld : public CBaseEntity
{
public:
	DECLARE_CLASS( CWorld, CBaseEntity );

	CWorld();
	~CWorld();

	DECLARE_SERVERCLASS();

	virtual int RequiredEdictIndex( void ) { return 0; }   // the world always needs to be in slot 0
	
	static void RegisterSharedActivities( void );
	static void RegisterSharedEvents( void );
	virtual void Spawn( void );
	virtual void Precache( void );
	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	virtual void DecalTrace( trace_t *pTrace, char const *decalName );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ) {}
	virtual void VPhysicsFriction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit ) {}

	// save/restore
	// only overload these if you have special data to serialize
	virtual int	Save(ISave& save);
	virtual int	Restore(IRestore& restore);

	// handler to reset stuff after you are restored
	// called after all entities have been loaded from all affected levels
	// called before activate
	// NOTE: Always chain to base class when implementing this!
	virtual void OnRestore();

	inline void GetWorldBounds( Vector &vecMins, Vector &vecMaxs )
	{
		VectorCopy( m_WorldMins, vecMins );
		VectorCopy( m_WorldMaxs, vecMaxs );
	}

	inline float GetWaveHeight() const
	{
		return (float)m_flWaveHeight;
	}

	bool GetDisplayTitle() const;
	bool GetStartDark() const;
	bool GetWorldEpisodic() const { return GetWorldFlag(WORLD_IS_EPISODIC); }

	void SetDisplayTitle( bool display );
	void SetStartDark( bool startdark );

	bool IsColdWorld( void );

	const char *GetPopulationTag() const
	{
		return !m_iszPopulationTag ? nullptr : STRING(m_iszPopulationTag);
	}

	int		GetMapVersion() { return m_nMapVersion; }

	bool GetWorldFlag(int iFlag, bool* bDefined = nullptr) const;
protected:
	void SetWorldFlag(int iFlag, bool bValue);
	bool GetWorldFlagDefault(int iFlag) const;

private:
	DECLARE_DATADESC();

	string_t m_iszChapterTitle;
	string_t m_iszPopulationTag;

	CNetworkVar( float, m_flWaveHeight );
	CNetworkVector( m_WorldMins );
	CNetworkVector( m_WorldMaxs );
	CNetworkVar( float, m_flMaxOccludeeArea );
	CNetworkVar( float, m_flMinOccluderArea );
	CNetworkVar( float, m_flMinPropScreenSpaceWidth );
	CNetworkVar( float, m_flMaxPropScreenSpaceWidth );
	CNetworkVar( string_t, m_iszDetailSpriteMaterial );

	int		m_nMapVersion;

	// start flags
	CNetworkVar( bool, m_bStartDark );
	CNetworkVar( bool, m_bColdWorld );
	//bool m_bDisplayTitle;
	CBitVec<NUM_WORLD_BOOLS> m_bitWorldFlagBools;
	CBitVec<NUM_WORLD_BOOLS> m_bitWorldFlagDefs;
};


CWorld* GetWorldEntity();
extern const char *GetDefaultLightstyleString( int styleIndex );


#endif // WORLD_H
