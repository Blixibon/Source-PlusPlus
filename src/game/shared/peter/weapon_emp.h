#ifndef WEAPON_EMP_H
#define WEAPON_EMP_H
//#include "basecombatweapon.h"
//#include "basehlcombatweapon.h"

abstract_class IEMPInteractable
{
public:
	virtual bool EmpCanInteract(CBaseCombatWeapon *) = 0;
	virtual void EmpNotifyInteraction(CBaseCombatWeapon *) = 0;
};



#endif // !WEAPON_EMP_H

