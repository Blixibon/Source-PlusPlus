#ifndef C_HLMS_PLAYER_H
#define C_HLMS_PLAYER_H
#pragma once
#include "c_basehlplayer.h"
#include "c_portal_player.h"
#include "laz_player_shared.h"
#include "networkstringtabledefs.h"
#include "hl2_vehicle_radar.h"
#include "iinput.h"
#include "c_npc_manhack.h"

extern INetworkStringTable *g_pStringTablePlayerFootSteps;

class C_Laz_PlayerLocalData
{
public:
	DECLARE_CLASS_NOBASE(C_Laz_PlayerLocalData);
	DECLARE_EMBEDDED_NETWORKVAR();

	C_Laz_PlayerLocalData();

	float m_flTapePos[LOCATOR_MAX_CONTACTS];

	int m_iNumLocatorContacts;
	EHANDLE m_hLocatorEntities[LOCATOR_MAX_CONTACTS];
	Vector m_vLocatorPositions[LOCATOR_MAX_CONTACTS];
	int m_iLocatorContactType[LOCATOR_MAX_CONTACTS];
	float m_flLocatorRange;

	EHANDLE m_hSetOfManhacks[NUMBER_OF_CONTROLLABLE_MANHACKS];
};

class C_Laz_Player : public C_Portal_Player
{
public:
	DECLARE_CLASS(C_Laz_Player, C_Portal_Player);
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	//C_Laz_Player();

	virtual bool ShouldDoPortalRenderCulling();

	virtual void OnPreDataChanged(DataUpdateType_t updateType);
	virtual void			OnDataChanged(DataUpdateType_t updateType);
	virtual void			ClientThink();
	virtual float GetFOV( void );
	virtual const QAngle& GetRenderAngles();

	virtual void	UpdateFlashlight(void);
	void	PerformAutoMovement();

	virtual void	CalcViewModelView(const Vector& eyeOrigin, const QAngle& eyeAngles);

	//bool	IsInReload();
	//int		GetHideBits();
	void		UpdateOnRemove();

	Vector 	GetPlayerEyeHeight(void);

	void AvoidPlayers(CUserCmd * pCmd);

	virtual bool CreateMove(float flInputSampleTime, CUserCmd *pCmd);

	virtual float			GetMinFOV()	const { return 20.0f; }

	virtual int DrawModel(int flags);

	virtual CStudioHdr* OnNewModel(void);
	virtual void CalculateIKLocks(float currentTime);

	// Shadows
	virtual ShadowType_t ShadowCastType(void);
	virtual void GetShadowRenderBounds(Vector &mins, Vector &maxs, ShadowType_t shadowType);
	virtual void GetRenderBounds(Vector& theMins, Vector& theMaxs);
	virtual bool GetShadowCastDirection(Vector *pDirection, ShadowType_t shadowType) const;
	virtual bool ShouldReceiveProjectedTextures(int flags);

	// override in sub-classes
	virtual void DoAnimationEvents(CStudioHdr* pStudio);

	enum
	{
		HIDEARM_LEFT = 0x01,
		HIDEARM_RIGHT = 0x02
	};

	bool	IsInReload();
	int		GetHideBits();

	virtual void				BuildFirstPersonMeathookTransformations(CStudioHdr* hdr, Vector* pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList& boneComputed, const char* pchHeadBoneName);

	virtual	bool		ShouldCollide(int collisionGroup, int contentsMask) const;

	virtual void PlayStepSound(const Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force);
	void	PrecacheFootStepSounds(void);
	const char *GetPlayerModelSoundPrefix(void);

	virtual	bool			TestHitboxes(const Ray_t &ray, unsigned int fContentsMask, trace_t& tr);

	virtual void PostThink(void);
	virtual void PreThink(void);

	// Taunts/VCDs
	virtual bool	StartSceneEvent(CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, C_BaseEntity *pTarget);
	virtual void	CalcView(Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov);
	bool			StartGestureSceneEvent(CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget);
	void			TurnOnTauntCam(void);
	void			TurnOffTauntCam(void);
	bool			InTauntCam(void) { return m_bWasTaunting; }
	//virtual void	ThirdPersonSwitch(bool bThirdperson);

	//C_BaseAnimating *m_pLegs;

	bool	CanSprint(void);
	void	StartSprinting(void);
	void	StopSprinting(void);
	void	HandleSpeedChanges(void);

	// Walking
	void StartWalking(void);
	void StopWalking(void);
	bool IsWalking(void) { return m_fIsWalking; }

	int			GetMovementConfig();
	const LazSpeedData_t GetLazMoveData();

	//virtual void PostThink(void);

	C_NPC_Manhack* GetCurrentManhack();
	int				GetManhackCount();
public:
	bool m_bHasLongJump;

	int m_iPlayerSoundType;

	C_Laz_PlayerLocalData m_LazLocal;

protected:
	void HandleTaunting(void);
	bool GetIntervalMovement(float flIntervalUsed, bool& bMoveSeqFinished, Vector& newPosition, QAngle& newAngles);

	bool				m_bWasTaunting;
	CameraThirdData_t	m_TauntCameraData;

	QAngle				m_angTauntPredViewAngles;
	QAngle				m_angTauntEngViewAngles;

	bool m_fIsWalking;

	CBitVec<MAXSTUDIOBONES> m_bitLeftArm;
	CBitVec<MAXSTUDIOBONES> m_bitRightArm;
	CBitVec<MAXSTUDIOBONES> m_bitHair;
	int						m_iHeadBone;

	int					m_nFlashlightType;
	int					m_nMovementCfg;

	float				m_flEyeHeightOverride;

	bool m_bInAutoMovement;
	QAngle m_angAutoMoveAngles;

	int				m_iOldTeam;

	EHANDLE			m_hCurrentManhack;
};

inline C_Laz_Player *ToLazuulPlayer(C_BaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
		return NULL;

	return dynamic_cast<C_Laz_Player*>(pEntity);
}

class IPlayerColorConvars
{
public:
	virtual ConVar* GetColorConVar(int iColor) = 0;
};

extern IPlayerColorConvars* g_pColorConvars;

#endif