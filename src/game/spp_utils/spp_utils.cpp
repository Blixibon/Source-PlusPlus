#include "spp_utils.h"
#include "mapedit_helper.h"
#include "tier2/tier2.h"

class CGameSharedUtils : public CTier2AppSystem<IGameSharedUtils>
{
	typedef CTier2AppSystem<IGameSharedUtils> BaseClass;
public:
	CGameSharedUtils() : BaseClass(true)
	{
		m_bConnected = false;
	}

	virtual bool Connect( CreateInterfaceFn factory ) 
	{
		if (m_bConnected)
			return true;

		m_bConnected = BaseClass::Connect(factory);
		return m_bConnected;
	}
	virtual void Disconnect()
	{
		if (!m_bConnected)
			return;

		BaseClass::Disconnect();
		m_bConnected = false;
	}

	virtual IMapEditHelper* GetMapEditHelper();

private:
	bool m_bConnected;
};

CGameSharedUtils g_SharedUtils;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameSharedUtils, IGameSharedUtils, SPP_UTILS_INTERFACE, g_SharedUtils);

IMapEditHelper* CGameSharedUtils::GetMapEditHelper()
{
	static CMapEditHelper s_Helper;
	return &s_Helper;
}
