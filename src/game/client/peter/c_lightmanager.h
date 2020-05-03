#pragma once
#include "igamesystem.h"
#include "utldict.h"
#include "utllinkedlist.h"
#include "utlvector.h"
#include "dlight.h"

class CManagedLight;

class CDLightManager : public CAutoGameSystemPerFrame
{
public:
	CDLightManager() : CAutoGameSystemPerFrame("DLightManager")
	{}

	// Init, shutdown
	// return true on success. false to abort DLL init!
	virtual bool Init();
	virtual void PostInit() {}
	virtual void Shutdown() {}

	// Level init, shutdown
	virtual void LevelInitPreEntity() {}
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreClearSteamAPIContext() {}
	virtual void LevelShutdownPreEntity() {}
	virtual void LevelShutdownPostEntity() {}

	virtual void OnSave() {}
	virtual void OnRestore() {}
	virtual void SafeRemoveIfDesired() {}

	// Called before rendering
	virtual void PreRender() { }

	// Gets called each frame
	virtual void Update(float frametime);

	// Called after rendering
	virtual void PostRender() { }

	CManagedLight* CreateLight(const char* pszLightType, const Vector *pOrigin = nullptr);

	void	CreateAutoFollowLight(const CEffectData& data);
	void	CreateAutoFollowLight(C_BaseEntity *pEntity, int iAttachmentIndex, int r, int g, int b, int exponent, float radius, float time, float decay);

protected:
	typedef struct lighttype_s
	{
		Color color;
		float flRadius;
		int iLightStyle;

		float flSustainTime;
		float flDecayTime;
	} lighttype_t;

	CUtlDict<lighttype_t> m_LightTypes;
	CUtlVector< CSmartPtr<CManagedLight> > m_ActiveLights;
	dlight_t* m_pDLights[16];
};

class CManagedLight
{
public:
	CManagedLight()
	{
		m_iRefCount = 0;
		m_flOriginalRadius = 0.f;
		m_flRadiusScale = 1.f;
		m_DeathTimes.range = 0.f;
		m_DeathTimes.start = 0.f;
	}

	virtual void UpdateOrigin(Vector vecOrigin);
	virtual bool IsDead() { return gpGlobals->curtime > m_DLightData.m_DataCopy.die; }

	virtual void AddRef() { m_iRefCount++; }
	virtual void Release();

	const Vector& GetOrigin() { return m_DLightData.m_DataCopy.origin; }
	void	CopyIntoLight(dlight_t* pLight);

	virtual void	StartDecay();
	virtual void	SetRadiusScale(float flNewScale) { m_flRadiusScale = flNewScale; }

protected:
	virtual void	Update();

	class CLightData
	{
	public:
		dlight_t m_DataCopy;
		dlight_t* m_pLight;
		float	m_flLastRadius;

		CLightData();
		void Update(dlight_t* pLight);
	};

	CLightData	m_DLightData;
	float		m_flOriginalRadius;
	float		m_flRadiusScale;

	interval_t	m_DeathTimes;

	friend class CDLightManager;
private:
	int m_iRefCount;
};

class CAutoFollowLight : public CManagedLight
{
public:
	CAutoFollowLight() : CManagedLight()
	{
		m_hFollowEntity.Term();
		m_iAttachmentIndex = -1;
	}

protected:
	virtual void	Update();

	EHANDLE m_hFollowEntity;
	int	m_iAttachmentIndex;

	friend class CDLightManager;
};

CDLightManager* LightManager();