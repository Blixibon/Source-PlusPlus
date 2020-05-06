#include "cbase.h"
#include "vgui_controls/Frame.h"
#include "c_baseanimating.h"

enum ControlSlots_e
{
	SLOT_SHIELDS = 0,
	SLOT_TURRETS,
	SLOT_MANHACKS,
	SLOT_DOORS,

	NUM_CONTROL_SLOTS
};

class C_HLSSCombineInterface : public C_BaseAnimating
{
public:
	DECLARE_CLASS(C_HLSSCombineInterface, C_BaseAnimating);
	DECLARE_CLIENTCLASS();

	bool	m_bSlotActive[NUM_CONTROL_SLOTS];
	bool	m_bSlotIsAvailable[NUM_CONTROL_SLOTS];
};

IMPLEMENT_CLIENTCLASS_DT(C_HLSSCombineInterface, DT_HLSSCombineInterface, CHLSSCombineInterface)
RecvPropArray3(RECVINFO_ARRAY(m_bSlotActive), RecvPropBool(RECVINFO(m_bSlotActive[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_bSlotIsAvailable), RecvPropBool(RECVINFO(m_bSlotIsAvailable[0]))),
END_RECV_TABLE();