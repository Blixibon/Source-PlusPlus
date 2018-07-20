#include "cbase.h"
#include "skydome.h"
#include "view.h"
#include "model_types.h"

CSkydomeManager skydome;
CSkydomeManager *g_pSkydome = &skydome;

ConVar r_skydome("r_skydome", "1", FCVAR_ARCHIVE);

class CDomeModel : public C_BaseAnimating
{
public:
	DECLARE_CLASS(CDomeModel, C_BaseAnimating);

	RenderGroup_t GetRenderGroup()
	{
		return RENDER_GROUP_OTHER;
	}

	virtual int DrawModel(int flags);
};

int CDomeModel::DrawModel(int flags)
{
	if (!g_pSkydome->IsReadyToDraw())
		return 0;

	if (flags & STUDIO_RENDER)
		modelrender->ForcedMaterialOverride(g_pSkydome->GetSkyMaterial());

	int iRet = BaseClass::DrawModel(flags);

	if (flags & STUDIO_RENDER)
		modelrender->ForcedMaterialOverride(NULL);

	return iRet;
}

void CSkydomeManager::LevelInitPostEntity()
{
	m_pDomeEnt = new CDomeModel;
	m_pDomeEnt->InitializeAsClientEntity("models/props_skydomes/skydome.mdl", RENDER_GROUP_OTHER);

	//m_pDomeEnt->SetModelScale(16.0f);

	m_iPlacementAttach = m_pDomeEnt->LookupAttachment("placementOrigin");
	
	ConVarRef skyname("sv_skyname");

	char szSkyDome[MAX_PATH];
	Q_snprintf(szSkyDome, sizeof(szSkyDome), "skybox/%s_dome", skyname.GetString());

	IMaterial *pMat = materials->FindMaterial(szSkyDome, NULL, false);
	
	m_SkyMat.Init(pMat);
}

void CSkydomeManager::LevelShutdownPreEntity()
{
	if (m_pDomeEnt != nullptr)
	{
		m_pDomeEnt->SUB_Remove();
		m_pDomeEnt = nullptr;
	}
}

void CSkydomeManager::Update(float frametime)
{
	ConVarRef skyname("sv_skyname");

	char szSkyDome[MAX_PATH];
	Q_snprintf(szSkyDome, sizeof(szSkyDome), "skybox/%s_dome", skyname.GetString());

	IMaterial *pMat = materials->FindMaterial(szSkyDome, NULL, false);

	m_SkyMat.Init(pMat);
}

void CSkydomeManager::PreRender()
{
	m_pDomeEnt->SetLocalOrigin(CurrentViewOrigin());
	m_pDomeEnt->SetLocalAngles(vec3_angle);
}

bool CSkydomeManager::IsReadyToDraw()
{
	if (m_pDomeEnt == nullptr)
		return false;

	if (!m_pDomeEnt->GetModelPtr())
		return false;

	if (IsErrorMaterial(m_SkyMat))
		return false;

	if (!r_skydome.GetBool())
		return false;

	return true;
}

//void CSkydomeManager::DrawSkydome()
//{
//	if (!IsReadyToDraw())
//		return;
//
//	modelrender->ForcedMaterialOverride(m_SkyMat);
//
//	m_pDomeEnt->DrawModel(STUDIO_RENDER);
//
//	modelrender->ForcedMaterialOverride(NULL);
//}