#pragma once
#include "shareddefs.h"

class ILocalAnnouncer
{
public:
    virtual void DispatchAnnouncement(int iAnnouncement) = 0;
};

ILocalAnnouncer* GetAnnouncer();