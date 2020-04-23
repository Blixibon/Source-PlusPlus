#include "cbase.h"
#include "basehlcombatweapon_shared.h"
#include "basecombatcharacter.h"


//-----------------------------------------------------------------------------
// Crossbow Bolt
//-----------------------------------------------------------------------------
class CShockPlasma : public CBaseCombatCharacter
{
	DECLARE_CLASS(CShockPlasma, CBaseCombatCharacter);

public:
	/*CShockPlasma() { };
	~CShockPlasma();*/

	Class_T Classify(void) { return CLASS_NONE; }

public:
	void Spawn(void);
	void Precache(void);
	void BubbleThink(void);
	void BoltTouch(CBaseEntity *pOther);
	bool CreateVPhysics(void);
	unsigned int PhysicsSolidMaskForEntity() const;
	static CShockPlasma *BoltCreate(const Vector &vecOrigin, const QAngle &angAngles, CBaseCombatCharacter *pentOwner = NULL);

protected:

	bool	CreateSprites(void);

	

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
};
