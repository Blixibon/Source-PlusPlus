#ifndef RENDER_EFFECTS_H
#define RENDER_EFFECTS_H
#pragma once

namespace RenderFxInternal
{
	class CRenderPart;
	class CRenderPartInstanceContainer;
	class IRenderPartProvider;

	struct renderPartBaseData_t
	{
		matrix3x4_t matTransform;
		Color	clrColor;
		char	szAttachmentOrBoneName[32];
	};

	class CRenderPartInstanceBase : public CDefaultClientRenderable
	{
		DECLARE_CLASS_GAMEROOT(CRenderPartInstanceBase, CDefaultClientRenderable)
	public:
		virtual int	DrawModel(int flags) { return 1; }

		virtual void	Init(CRenderPart *pDef, CRenderPartInstanceContainer *pContainer)
		{
			m_pDefinition = pDef;
			m_pContainer = pContainer;
			m_bUseBone = false;

			const char *pchBoneOrAttchmentName = pDef->GetPartData()->szAttachmentOrBoneName;
			m_iAttachmentOrBoneId = pContainer->GetParent()->LookupAttachment(pchBoneOrAttchmentName);
			if (m_iAttachmentOrBoneId <= 0)
			{
				m_iAttachmentOrBoneId = pContainer->GetParent()->LookupBone(pchBoneOrAttchmentName);
				if (m_iAttachmentOrBoneId >= 0)
					m_bUseBone = true;
			}
		}

	protected:
		CRenderPart		*m_pDefinition;
		bool					 m_bUseBone;
		int						 m_iAttachmentOrBoneId;
		CRenderPartInstanceContainer	*m_pContainer;

	public:
		virtual void SetRefEHandle(const CBaseHandle &handle) { Assert(false); }
		virtual const CBaseHandle& GetRefEHandle() const { Assert(false); return *((CBaseHandle*)0); }

		virtual IClientUnknown*		GetIClientUnknown() { return this; }
		virtual ICollideable*		GetCollideable() { return 0; }
		virtual IClientRenderable*	GetClientRenderable() { return this; }
		virtual IClientNetworkable*	GetClientNetworkable() { return 0; }
		virtual IClientEntity*		GetIClientEntity() { return 0; }
		virtual C_BaseEntity*		GetBaseEntity() { return 0; }
		virtual IClientThinkable*	GetClientThinkable() { return 0; }

		virtual bool				IsTransparent() { return true; }
		virtual bool				ShouldDraw() { return m_pContainer->ShouldDraw(); }

		

		virtual const Vector &			GetRenderOrigin(void)
		{
			static Vector vecOrigin;
			MatrixPosition(RenderableToWorldTransform(), vecOrigin);
			return vecOrigin;
		}
		virtual const QAngle &			GetRenderAngles(void)
		{
			static QAngle Angles;
			MatrixAngles(RenderableToWorldTransform(), Angles);
			return Angles;
		}

		virtual const matrix3x4_t &		RenderableToWorldTransform()
		{
			static matrix3x4_t matResult;
			SetIdentityMatrix(matResult);
			matrix3x4_t matParent;
			bool bGotMatrix = false;
			if (m_bUseBone)
			{
				if (m_iAttachmentOrBoneId >= 0)
				{
					//C_BaseAnimating::AutoAllowBoneAccess(true, false);
					m_pContainer->GetParent()->GetBoneTransform(m_iAttachmentOrBoneId, matParent);
					bGotMatrix = true;
				}
			}
			else
			{
				if (m_iAttachmentOrBoneId > 0)
				{
					m_pContainer->GetParent()->GetAttachment(m_iAttachmentOrBoneId, matParent);
					bGotMatrix = true;
				}
			}

			if (!bGotMatrix)
			{
				matParent = m_pContainer->GetParent()->EntityToWorldTransform();
				bGotMatrix = true;
			}

			ConcatTransforms(m_pDefinition->GetPartData()->matTransform, matParent, matResult);
			return matResult;
		}
	};

	class CRenderPartInstanceContainer
	{
	public:
		C_BaseAnimating *GetParent() { return m_hParentEntity.Get(); }
		virtual bool				ShouldDraw() { return GetParent() ? GetParent()->ShouldDraw() : false; }
		void			AddInstance(CRenderPartInstanceBase *pInst)
		{
			m_vecInstances.AddToTail(pInst);
		}
	protected:
		CHandle<C_BaseAnimating> m_hParentEntity;
		CUtlVector<CRenderPartInstanceBase *> m_vecInstances;
	};

	class CRenderPart
	{
	public:
		//CRenderPart(const char *pchName);

		static CRenderPart *Create(renderPartBaseData_t *pData, IRenderPartProvider *pProvider)
		{

		}

		renderPartBaseData_t *GetPartData() { return m_pData; }

		void	SetupInstance(CRenderPartInstanceBase *pInst, CRenderPartInstanceContainer *pContainer);

		CRenderPartInstanceBase *CreateInstance() { return m_pInstanceProvider->CreateInstance(); }
	protected:
		renderPartBaseData_t *m_pData;
		IRenderPartProvider *m_pInstanceProvider;
	};

	class IRenderPartProvider
	{
		DECLARE_CLASS_NOBASE(IRenderPartProvider)
	public:
		IRenderPartProvider(const char *pchName);
		//~IRenderPartProvider();

		virtual CRenderPartInstanceBase *CreateInstance() = 0;

		virtual	renderPartBaseData_t *ParseDataFromKV(KeyValues *pKV) = 0;
		void ParseBaseDataFromKV(KeyValues *pKV, renderPartBaseData_t *pData);
	};

	typedef struct renderFilter_s {
		char model[MAX_PATH];
		int skin;
	} renderFilter_t;

	class CRenderEffect
	{
	public:

		CUtlVector<CRenderPart *> m_Parts;
		CUtlVector<renderFilter_t> m_Filters;
	};

	class CRenderEffectsExtension : public CAutoGameSystemPerFrame
	{
		DECLARE_CLASS_GAMEROOT(CRenderEffectsExtension, CAutoGameSystemPerFrame)
	public:
		virtual void PostInit();

		void	AddPartProvider(IRenderPartProvider *pProvider, const char *pchName)
		{
			m_PartMap[pchName] = pProvider;
		}

	protected:
		void		ParseRenderEffect(KeyValues *pkvEffect);

		CUtlStringMap<IRenderPartProvider *> m_PartMap;
		CUtlVector<CRenderEffect> m_vecEffects;
		CUtlVector<CRenderPartInstanceContainer> m_vecInstances;
	};
}

RenderFxInternal::CRenderEffectsExtension *RenderFXExtension();
#endif // !RENDER_EFFECTS_H
