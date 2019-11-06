#ifndef ENv_VOLUME_H
#define ENv_VOLUME_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "env_tonemap_controller.h"
#include "fogcontroller.h"
#include "colorcorrection.h"

class CEnvVolume : public CBaseTrigger
{
public:
	DECLARE_CLASS(CEnvVolume, CBaseTrigger)
	DECLARE_DATADESC()

	CEnvVolume();
	~CEnvVolume();

	virtual void Spawn(void);
	virtual void Activate();
	virtual void StartTouch(CBaseEntity* other);
	virtual void EndTouch(CBaseEntity* other);

	void Enable(void);
	void Disable(void);

	static CEnvVolume* FindEnvVolumeForPosition(const Vector& position, CBaseEntity *pLooker = nullptr);

	CBaseEntity* GetTonemapController(void) const
	{
		return m_hTonemapController.Get();
	}

	fogparams_t* GetFog(void)
	{
		if (m_hFogController.IsValid())
			return &m_hFogController->m_fog;
		return &m_fog;
	}

	const char* GetFogControllerName() const
	{
		return STRING(m_fogName);
	}

	CFogController* GetFogController() const
	{
		return m_hFogController.Get();
	}

	CColorCorrection* GetColorCorrectionController() const
	{
		return m_hColorCorrectionController.Get();
	}

private:
	string_t m_fogName;
	string_t m_colorCorrectionName;
	string_t m_tonemapControllerName;

	CHandle< CFogController > m_hFogController;
	CHandle< CColorCorrection > m_hColorCorrectionController;
	EHANDLE m_hTonemapController;

	fogparams_t	m_fog;

	bool m_bInFogVolumesList;

	void AddToGlobalList();
	void RemoveFromGlobalList();
};

extern CUtlVector< CEnvVolume* > TheEnvVolumes;

#endif
