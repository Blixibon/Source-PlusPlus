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
	void UpdateOrigin(Vector vecOrigin);
	bool IsDead() { return gpGlobals->curtime > m_DLightData.m_DataCopy.die; }

	void AddRef() { m_iRefCount++; }
	void Release();

	const Vector& GetOrigin() { return m_DLightData.m_DataCopy.origin; }
	void	CopyIntoLight(dlight_t* pLight);

	void	StartDecay();
	void	SetRadiusScale(float flNewScale) { m_flRadiusScale = flNewScale; }

protected:
	void	Update();

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

CDLightManager* LightManager();