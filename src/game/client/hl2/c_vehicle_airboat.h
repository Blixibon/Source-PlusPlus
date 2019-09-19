#ifndef C_VEHICLE_AIRBOAT_H
#define C_VEHICLE_AIRBOAT_H
#pragma once

#include "c_prop_vehicle.h"
#include "beamdraw.h"
#include "SpriteTrail.h"
#include "flashlighteffect.h"

#define AIRBOAT_DELTA_LENGTH_MAX	12.0f			// 1 foot
#define AIRBOAT_FRAMETIME_MIN		1e-6

#define HEADLIGHT_DISTANCE		1000

#define	MAX_WAKE_POINTS	16
#define	WAKE_POINT_MASK (MAX_WAKE_POINTS-1)

#define	WAKE_LIFETIME	0.5f

//=============================================================================
//
// Client-side Airboat Class
//
class C_PropAirboat : public C_PropVehicleDriveable
{
	DECLARE_CLASS(C_PropAirboat, C_PropVehicleDriveable);

public:

	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();
	DECLARE_DATADESC();

	C_PropAirboat();
	~C_PropAirboat();

public:

	// C_BaseEntity
	virtual void Simulate();

	// IClientVehicle
	virtual void UpdateViewAngles(C_BasePlayer* pLocalPlayer, CUserCmd* pCmd);
	virtual void OnEnteredVehicle(C_BasePlayer* pPlayer);
	virtual int GetPrimaryAmmoType() const;
	virtual int GetPrimaryAmmoClip() const;
	virtual bool PrimaryAmmoUsesClips() const;
	virtual int GetPrimaryAmmoCount() const;
	virtual int GetJoystickResponseCurve() const;

	int		DrawModel(int flags);

	// Draws crosshair in the forward direction of the boat
	void DrawHudElements();

private:

	void DrawPropWake(Vector origin, float speed);
	void DrawPontoonSplash(Vector position, Vector direction, float speed);
	void DrawPontoonWake(Vector startPos, Vector wakeDir, float wakeLength, float speed);

	void DampenEyePosition(Vector& vecVehicleEyePos, QAngle& vecVehicleEyeAngles);
	void DampenForwardMotion(Vector& vecVehicleEyePos, QAngle& vecVehicleEyeAngles, float flFrameTime);
	void DampenUpMotion(Vector& vecVehicleEyePos, QAngle& vecVehicleEyeAngles, float flFrameTime);
	void ComputePDControllerCoefficients(float* pCoefficientsOut, float flFrequency, float flDampening, float flDeltaTime);

	void UpdateHeadlight(void);
	void UpdateWake(void);
	int	 DrawWake(void);
	void DrawSegment(const BeamSeg_t& beamSeg, const Vector& vNormal);

	TrailPoint_t* GetTrailPoint(int n)
	{
		int nIndex = (n + m_nFirstStep) & WAKE_POINT_MASK;
		return &m_vecSteps[nIndex];
	}

private:

	Vector		m_vecLastEyePos;
	Vector		m_vecLastEyeTarget;
	Vector		m_vecEyeSpeed;
	Vector		m_vecTargetSpeed;

	float		m_flViewAngleDeltaTime;

	bool		m_bHeadlightIsOn;
	int			m_nAmmoCount;
	CHeadlightEffect* m_pHeadlight;

	int				m_nExactWaterLevel;

	TrailPoint_t	m_vecSteps[MAX_WAKE_POINTS];
	int				m_nFirstStep;
	int				m_nStepCount;
	float			m_flUpdateTime;

	TimedEvent		m_SplashTime;
	CMeshBuilder	m_Mesh;

	Vector			m_vecPhysVelocity;
};
#endif // !C_VEHICLE_AIRBOAT_H
