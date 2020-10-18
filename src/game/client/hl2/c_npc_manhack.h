#pragma once

#include "c_ai_basenpc.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "c_baseplayer.h"

enum ManhackEye_e
{
	MANHACK_EYE_STATE_IDLE,
	MANHACK_EYE_STATE_CHASE,
	MANHACK_EYE_STATE_CHARGE,
	MANHACK_EYE_STATE_STUNNED,
};


class C_ManhackSprite : public C_Sprite
{
public:
	DECLARE_CLASS(C_ManhackSprite, C_Sprite);

	virtual int		DrawModel(int flags)
	{
		if (m_bSupressDraw)
			return 0;

		return BaseClass::DrawModel(flags);
	}

	void	SetSupressDraw(bool bSupress) { m_bSupressDraw = bSupress; }

protected:
	bool m_bSupressDraw;
};

class C_NPC_Manhack : public C_AI_BaseNPC
{
public:
	C_NPC_Manhack();

	DECLARE_CLASS(C_NPC_Manhack, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();
	DECLARE_DATADESC();

	// Purpose: Start the manhack's engine sound.
	virtual void OnDataChanged(DataUpdateType_t type);
	virtual void UpdateOnRemove(void);
	virtual void OnRestore();

	virtual bool ShouldDraw();

	void SupressGlows(bool bSupress);

	C_BaseEntity* GetManhackTarget() { return m_hHUDTarget.Get(); }

private:
	C_NPC_Manhack(const C_NPC_Manhack&);

	// Purpose: Start + stop the manhack's engine sound.
	void SoundInit(void);
	void SoundShutdown(void);

	void SetEyeState(int state);
	void StartEye(void);
	void KillEye(void);

	CSoundPatch* m_pEngineSound1;
	CSoundPatch* m_pEngineSound2;
	CSoundPatch* m_pBladeSound;

	int				m_nEnginePitch1;
	int				m_nEnginePitch2;
	float			m_flEnginePitch1Time;
	float			m_flEnginePitch2Time;

	// Eye State
	int m_nEyeState;
	int m_nLastEyeState;

	C_ManhackSprite* m_pEyeGlow;
	C_ManhackSprite* m_pLightGlow;

	// Player Control
	bool	IsControlledByLocalPlayer();

	CHandle<CBasePlayer> m_hOwningPlayer;
	bool m_bIsControlled;
	bool m_bLastIsControlledByLocalPlayer;

	EHANDLE m_hHUDTarget;
};
