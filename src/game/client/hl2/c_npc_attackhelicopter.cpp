#include "cbase.h"
#include "c_basehelicopter.h"
#include "c_ai_spotlight.h"


class C_NPC_AttackHelicopter : public C_BaseHelicopter
{
public:
	DECLARE_CLASS(C_NPC_AttackHelicopter, C_BaseHelicopter);
	DECLARE_CLIENTCLASS();

	void AddEntity(void);

private:
	C_AI_Spotlight m_Spotlight;
};

IMPLEMENT_CLIENTCLASS_DT(C_NPC_AttackHelicopter, DT_AttackHelicopter, CNPC_AttackHelicopter)
RecvPropDataTable(RECVINFO_DT(m_Spotlight), 0, &REFERENCE_RECV_TABLE(DT_AISpotlight)),
END_RECV_TABLE()

void C_NPC_AttackHelicopter::AddEntity()
{
	BaseClass::AddEntity();

	m_Spotlight.ClientUpdate(this);
}