#ifndef IENERGYSHIELD_H
#define IENERGYSHIELD_H
#pragma once

class IEnergyShield
{
public:
	virtual bool IsShieldActive() = 0;
	virtual void SetShieldActive(bool) = 0;
};

#endif // !IENERGYSHIELD_H
