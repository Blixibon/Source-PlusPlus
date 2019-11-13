#include "cbase.h"
#include "c_linked_portal_door.h"
#include "view_scene.h"
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(linked_portal_door, C_LinkedPortalDoor);

IMPLEMENT_CLIENTCLASS_DT(C_LinkedPortalDoor, DT_LinkedPortalDoor, CLinkedPortalDoor)
END_RECV_TABLE();

BEGIN_DATADESC(C_LinkedPortalDoor)
END_DATADESC();

C_LinkedPortalDoor::C_LinkedPortalDoor()
	: BaseClass()
{
	
}

C_LinkedPortalDoor::~C_LinkedPortalDoor()
{

}

extern ConVar mat_wireframe;

void C_LinkedPortalDoor::DrawSimplePortalMesh(const IMaterial *pMaterialOverride, float fForwardOffsetModifier)
{
	const IMaterial *pMaterial;
	if(pMaterialOverride)
		pMaterial = pMaterialOverride;
	else
		pMaterial = m_Materials.m_PortalMaterials[m_bIsPortal2 ? 1 : 0];

	CMatRenderContextPtr pRenderContext(materials);
	pRenderContext->Bind((IMaterial *)pMaterial, GetClientRenderable());

	// This can depend on the Bind command above, so keep this after!
	UpdateFrontBufferTexturesForMaterial((IMaterial *)pMaterial);

	pRenderContext->MatrixMode(MATERIAL_MODEL); //just in case
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	Vector ptCenter = m_ptOrigin + (m_vForward * fForwardOffsetModifier);

	Vector verts[4];
	verts[0] = ptCenter + (m_vRight * m_flWidth) - (m_vUp * m_flHeight);
	verts[1] = ptCenter + (m_vRight * m_flWidth) + (m_vUp * m_flHeight);
	verts[2] = ptCenter - (m_vRight * m_flWidth) - (m_vUp * m_flHeight);
	verts[3] = ptCenter - (m_vRight * m_flWidth) + (m_vUp * m_flHeight);

	float vTangent[4] = {-m_vRight.x, -m_vRight.y, -m_vRight.z, 1.0f};

	CMeshBuilder meshBuilder;
	IMesh* pMesh = pRenderContext->GetDynamicMesh(false);
	meshBuilder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

	meshBuilder.Position3fv(&verts[0].x);
	meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
	meshBuilder.TexCoord2f(1, 0.0f, 0.0f);
	meshBuilder.Normal3f(m_vForward.x, m_vForward.y, m_vForward.z);
	meshBuilder.UserData(vTangent);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv(&verts[1].x);
	meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
	meshBuilder.TexCoord2f(1, 0.0f, 0.0f);
	meshBuilder.Normal3f(m_vForward.x, m_vForward.y, m_vForward.z);
	meshBuilder.UserData(vTangent);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv(&verts[2].x);
	meshBuilder.TexCoord2f(0, 1.0f, 1.0f);
	meshBuilder.TexCoord2f(1, 1.0f, 1.0f);
	meshBuilder.Normal3f(m_vForward.x, m_vForward.y, m_vForward.z);
	meshBuilder.UserData(vTangent);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv(&verts[3].x);
	meshBuilder.TexCoord2f(0, 1.0f, 1.0f);
	meshBuilder.TexCoord2f(1, 1.0f, 1.0f);
	meshBuilder.Normal3f(m_vForward.x, m_vForward.y, m_vForward.z);
	meshBuilder.UserData(vTangent);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	if(mat_wireframe.GetBool())
	{
		pRenderContext->Bind((IMaterial *)(const IMaterial *)g_pPortalRender->m_MaterialsAccess.m_Wireframe, (CPortalRenderable*)this);

		IMesh* pMesh = pRenderContext->GetDynamicMesh(false);
		meshBuilder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

		meshBuilder.Position3fv(&verts[0].x);
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.TexCoord2f(1, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3fv(&verts[1].x);
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.TexCoord2f(1, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3fv(&verts[2].x);
		meshBuilder.TexCoord2f(0, 1.0f, 1.0f);
		meshBuilder.TexCoord2f(1, 1.0f, 1.0f);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3fv(&verts[3].x);
		meshBuilder.TexCoord2f(0, 1.0f, 1.0f);
		meshBuilder.TexCoord2f(1, 1.0f, 1.0f);
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}

	pRenderContext->MatrixMode(MATERIAL_MODEL);
	pRenderContext->PopMatrix();
}