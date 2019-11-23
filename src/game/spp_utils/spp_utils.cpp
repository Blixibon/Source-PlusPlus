#include "spp_utils.h"
#include "mapedit_helper.h"
#include "holiday_event_system.h"
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

		m_bConnected = BaseClass::Connect(factory) && m_Holidays.Init();

		return m_bConnected;
	}
	virtual void Disconnect()
	{
		if (!m_bConnected)
			return;

		m_Holidays.Shutdown();
		BaseClass::Disconnect();
		m_bConnected = false;
	}

	virtual const char* DoMapEdit(const char* pMapName, const char* pMapEntities, CUtlVector<char*>& vecVariants);
	virtual IHolidayEvents* GetEventSystem() { return &m_Holidays; }

private:
	bool m_bConnected;
	CMapEditHelper m_Helper;
	CHolidayEventSystem m_Holidays;
};

CGameSharedUtils g_SharedUtils;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameSharedUtils, IGameSharedUtils, SPP_UTILS_INTERFACE, g_SharedUtils);

const char* CGameSharedUtils::DoMapEdit(const char* pMapName, const char* pMapEntities, CUtlVector<char*>& vecVariants)
{
	return m_Helper.DoMapEdit(pMapName, pMapEntities, vecVariants);
}
