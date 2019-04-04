#include "cbase.h"
#include "render_effects.h"
#include "iviewrender_beams.h"

using namespace RenderFxInternal;



void RenderFxInternal::CRenderPart::SetupInstance(RenderFxInternal::CRenderPartInstanceBase *pInst, CRenderPartInstanceContainer *pContainer)
{
	pInst->Init(this, pContainer);

	// Hook it into the rendering system...
	ClientLeafSystem()->AddRenderable(pInst, RENDER_GROUP_TRANSLUCENT_ENTITY);
}

void RenderFxInternal::IRenderPartProvider::ParseBaseDataFromKV(KeyValues *pKV, RenderFxInternal::renderPartBaseData_t *pData)
{
	if (pData == nullptr)
		return;

	Vector vecOrigin;
	UTIL_StringToVector(vecOrigin.Base(), pKV->GetString("position", "0 0 0"));
	QAngle angRotation;
	UTIL_StringToVector(angRotation.Base(), pKV->GetString("angle", "0 0 0"));

	AngleMatrix(angRotation, vecOrigin, pData->matTransform);

	pData->clrColor = pKV->GetColor("color");

	const char *pchAttachmentOrBoneName = pKV->GetString("bone");
	Q_strncpy(pData->szAttachmentOrBoneName, pchAttachmentOrBoneName, 32);
}

class CRenderPartGlowDef;

struct renderPartGlowData_s : public RenderFxInternal::renderPartBaseData_t
{
	float size;
	bool trail;
};

class CRenderPartGlowInst : public RenderFxInternal::CRenderPartInstanceBase
{
	DECLARE_CLASS_GAMEROOT(CRenderPartGlowInst, RenderFxInternal::CRenderPartInstanceBase);
public:
	virtual int	DrawModel(int flags);

	virtual void	Init(RenderFxInternal::CRenderPart *pDef, RenderFxInternal::CRenderPartInstanceContainer *pContainer);

	renderPartGlowData_s *GetGlowData()
	{
		return static_cast<renderPartGlowData_s *> (m_pDefinition->GetPartData());
	}

	virtual void	GetRenderBounds(Vector& mins, Vector& maxs)
	{
		AddPointToBounds(Vector(GetGlowData()->size/2), mins, maxs);
		AddPointToBounds(Vector(-GetGlowData()->size / 2), mins, maxs);
	}
protected:
	Beam_t	*m_pTrail;
	BeamInfo_t m_BeamInfo;
};

void CRenderPartGlowInst::Init(RenderFxInternal::CRenderPart *pDef, RenderFxInternal::CRenderPartInstanceContainer *pContainer)
{
	BaseClass::Init(pDef, pContainer);

	if (GetGlowData()->trail)
	{
		m_BeamInfo.m_pStartEnt = pContainer->GetParent();
		m_BeamInfo.m_nType = TE_BEAMFOLLOW;
		m_BeamInfo.m_vecStart = GetRenderOrigin();
		m_BeamInfo.m_vecEnd = vec3_origin;

		m_BeamInfo.m_pszModelName = "trails/laser.vmt";

		m_BeamInfo.m_flHaloScale = 0.0f;
		m_BeamInfo.m_flLife = 0.0f;

		
		m_BeamInfo.m_flWidth = 8.0f;
		m_BeamInfo.m_flEndWidth = 0.0f;
		

		m_BeamInfo.m_flFadeLength = 0.0f;
		m_BeamInfo.m_flAmplitude = 25.0f;
		m_BeamInfo.m_flBrightness = 255.0;
		m_BeamInfo.m_flSpeed = 1.0f;
		m_BeamInfo.m_nStartFrame = 0.0;
		m_BeamInfo.m_flFrameRate = 30.0;
		m_BeamInfo.m_flRed = GetGlowData()->clrColor.r();
		m_BeamInfo.m_flGreen = GetGlowData()->clrColor.g();
		m_BeamInfo.m_flBlue = GetGlowData()->clrColor.b();
		m_BeamInfo.m_nSegments = 5;
		m_BeamInfo.m_bRenderable = false;
		m_BeamInfo.m_nFlags = (FBEAM_FOREVER);

		m_pTrail = beams->CreateBeamFollow(m_BeamInfo);
	}
}

int CRenderPartGlowInst::DrawModel(int flags)
{
	if (m_pTrail)
	{
		m_BeamInfo.m_vecStart = GetRenderOrigin();
		beams->UpdateBeamInfo(m_pTrail, m_BeamInfo);
		beams->DrawBeam(m_pTrail);
	}
}

class CRenderPartGlowProvider : public RenderFxInternal::IRenderPartProvider
{
	DECLARE_CLASS_GAMEROOT(CRenderPartGlowProvider, RenderFxInternal::IRenderPartProvider);
public:
	CRenderPartGlowProvider() : BaseClass("glow")
	{

	}

	virtual RenderFxInternal::CRenderPartInstanceBase *CreateInstance()
	{ 
		return new CRenderPartGlowInst();
	}

	virtual	RenderFxInternal::renderPartBaseData_t *ParseDataFromKV(KeyValues *pKV)
	{
		renderPartGlowData_s *pData = new renderPartGlowData_s;

		ParseBaseDataFromKV(pKV, pData);

		pData->size = pKV->GetFloat("size", 64.0f);
		pData->trail = pKV->GetBool("trail");
	}
};

static CRenderPartGlowProvider s_GlowProv;

IRenderPartProvider::IRenderPartProvider(const char *pchName)
{
	RenderFXExtension()->AddPartProvider(this, pchName);
}

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
RenderFxInternal::CRenderEffectsExtension *RenderFXExtension()
{
	static RenderFxInternal::CRenderEffectsExtension s_RemderFXE;
	return &s_RemderFXE;
}

void CRenderEffectsExtension::ParseRenderEffect(KeyValues *pkvEffect)
{
	m_vecEffects.AddToTail();
	CRenderEffect &hEffect = m_vecEffects.Tail();

	KeyValues *pkvFilters = pkvEffect->FindKey("filters");
	for (KeyValues *pkvFilter = pkvFilters->GetFirstTrueSubKey(); pkvFilter != NULL; pkvFilter = pkvFilter->GetFirstTrueSubKey())
	{
		hEffect.m_Filters.AddToTail();
		renderFilter_t &hFilter = hEffect.m_Filters.Tail();
		hFilter.skin = pkvFilter->GetInt("skin");
		Q_strncpy(hFilter.model, pkvFilter->GetString("model"), sizeof(hFilter.model));
	}


}