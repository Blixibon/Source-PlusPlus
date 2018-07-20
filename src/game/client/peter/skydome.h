#ifndef SKYDOME_H
#define SKYDOME_H

#include "igamesystem.h"

class C_BaseAnimating;

class CSkydomeManager : public CAutoGameSystemPerFrame
{
public:
	DECLARE_CLASS_GAMEROOT(CSkydomeManager, CAutoGameSystemPerFrame);

	CSkydomeManager() : BaseClass("CSkydomeManager")
	{}

	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPreEntity();
	virtual void PreRender();

	// Gets called each frame
	virtual void Update(float frametime);

	bool		IsReadyToDraw();
	//void		DrawSkydome();

	IMaterial *GetSkyMaterial()
	{
		return m_SkyMat;
	}

	C_BaseAnimating *GetDomeModel()
	{
		return m_pDomeEnt;
	}

protected:
	C_BaseAnimating *m_pDomeEnt;
	CMaterialReference m_SkyMat;
	int					m_iPlacementAttach;
};


extern CSkydomeManager *g_pSkydome;

#endif // !SKYDOME_H

