#ifndef C_HLMS_PLAYER_H
#define C_HLMS_PLAYER_H
#include "c_basehlplayer.h"
//#include "c_portal_player.h"
#include "laz_player_shared.h"
#include "networkstringtabledefs.h"
#include "iinput.h"

extern INetworkStringTable *g_pStringTablePlayerFootSteps;

class C_Laz_Player : public C_BaseHLPlayer
{
public:
	DECLARE_CLASS(C_Laz_Player, C_BaseHLPlayer);
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	//C_Laz_Player();


	virtual void			OnDataChanged(DataUpdateType_t updateType);
	virtual void			ClientThink();

	//bool	IsInReload();
	//int		GetHideBits();
	void		UpdateOnRemove();

	virtual bool CreateMove(float flInputSampleTime, CUserCmd *pCmd);

	// Shadows
	virtual ShadowType_t ShadowCastType(void);
	virtual void GetShadowRenderBounds(Vector &mins, Vector &maxs, ShadowType_t shadowType);
	virtual void GetRenderBounds(Vector& theMins, Vector& theMaxs);
	virtual bool GetShadowCastDirection(Vector *pDirection, ShadowType_t shadowType) const;
	virtual bool ShouldReceiveProjectedTextures(int flags);

	virtual void PlayStepSound(const Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force);
	void	PrecacheFootStepSounds(void);
	const char *GetPlayerModelSoundPrefix(void);

	virtual	bool			TestHitboxes(const Ray_t &ray, unsigned int fContentsMask, trace_t& tr);

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

	//virtual void PostThink(void);

	bool m_bHasLongJump;

	int m_iPlayerSoundType;

protected:
	void HandleTaunting(void);

	bool				m_bWasTaunting;
	CameraThirdData_t	m_TauntCameraData;

	QAngle				m_angTauntPredViewAngles;
	QAngle				m_angTauntEngViewAngles;

	bool m_fIsWalking;
};

#endif