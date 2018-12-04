#include "cbase.h"

EXTERN_RECV_TABLE(DT_AISpotlight);

class CSpotlightEffect;

enum AISPOT_FLAGS
{
	AI_SPOTLIGHT_NO_DLIGHTS = 0x1,
	AI_SPOTLIGHT_ENABLE_PROJECTED = 0x2,
};

class C_AI_Spotlight
{
public:
	DECLARE_CLASS_NOBASE(C_AI_Spotlight);
	DECLARE_EMBEDDED_NETWORKVAR();

	C_AI_Spotlight()
	{
		memset(this, 0, sizeof(ThisClass));
	}

	~C_AI_Spotlight();
	

	void	ClientUpdate(C_BaseAnimating *pOwner);

	EHANDLE m_hSpotlightTarget;
	int m_nSpotlightAttachment;
	int m_nFlags;
	CSpotlightEffect *m_pSpotLight;
};