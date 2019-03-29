#ifndef IENERGYSHIELD_H
#define IENERGYSHIELD_H
#pragma once

typedef enum {
	SHIELD_TYPE_PERSONAL = 0,
	SHIELD_TYPE_PROJECTED,
} ShieldType_t;

class IEnergyShield
{
public:
	virtual bool IsShieldActive() = 0;
	virtual void SetShieldActive(bool) = 0;

	virtual ShieldType_t GetShieldType() = 0;
};

#endif // !IENERGYSHIELD_H
