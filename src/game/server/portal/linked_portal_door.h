#ifndef _LINKED_PORTAL_DOOR_H_
#define _LINKED_PORTAL_DOOR_H_

#pragma once

#include "prop_portal.h"

class CLinkedPortalDoor : public CProp_Portal
{
	public:
		DECLARE_CLASS(CLinkedPortalDoor, CProp_Portal);
		DECLARE_SERVERCLASS();
		DECLARE_DATADESC();

		CLinkedPortalDoor();
		virtual ~CLinkedPortalDoor() OVERRIDE;

		virtual bool TestCollision(const Ray_t &ray, unsigned int fContentsMask, trace_t &tr) OVERRIDE;
		virtual void OnRestore() OVERRIDE;
		virtual void NewLocation(const Vector &vOrigin, const QAngle &qAngles) OVERRIDE;
		virtual void InputSetActivatedState(inputdata_t &inputdata) OVERRIDE;
		virtual void DoFizzleEffect(int iEffect, bool bDelayedPos) OVERRIDE;
		virtual void Activate() OVERRIDE;
		virtual void ResetModel() OVERRIDE;

	private:
		
};

#endif