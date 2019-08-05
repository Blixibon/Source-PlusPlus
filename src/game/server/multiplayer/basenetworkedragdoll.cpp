#include "cbase.h"
#include "basenetworkedragdoll.h"

LINK_ENTITY_TO_CLASS( networked_ragdoll, CBaseNetworkedRagdoll );

IMPLEMENT_SERVERCLASS_ST( CBaseNetworkedRagdoll, DT_BaseNetworkedRagdoll )
SendPropVector(SENDINFO(m_vecRagdollOrigin), -1, SPROP_COORD),
SendPropEHandle(SENDINFO(m_hPlayer)),
SendPropModelIndex(SENDINFO(m_nModelIndex)),
SendPropInt(SENDINFO(m_nForceBone), 8, 0),
SendPropVector(SENDINFO(m_vecForce), -1, SPROP_NOSCALE),
SendPropVector(SENDINFO(m_vecRagdollVelocity), 13, SPROP_ROUNDDOWN, -2048.0f, 2048.0f),
SendPropBool(SENDINFO(m_bGib)),
SendPropBool(SENDINFO(m_bBurning)),
SendPropExclude("DT_BaseEntity", "m_nRenderFX"),
END_SEND_TABLE()
