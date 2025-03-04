//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iv�n Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017
//
// Purpose: It is in charge of managing the memory of the entities 
// and the memory of the information besides considering who will be the best enemy of the bot.
// 
// If you are an expert in C++ and pointers, please get this code out of misery.

#include "cbase.h"
#include "bots\bot.h"
#include "bots\bot_manager.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#include "in_player.h"
#else
#include "in\in_utils.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Logging System
// Only for the current file, this should never be in a header.
//================================================================================

//#define Msg(...) Log_Msg(LOG_BOTS, __VA_ARGS__)
//#define Warning(...) Log_Warning(LOG_BOTS, __VA_ARGS__)

//================================================================================
//================================================================================
void CBotMemory::Update()
{
	VPROF_BUDGET("CBotMemory::Update", VPROF_BUDGETGROUP_BOTS_EXPENSIVE);

	if (!IsEnabled())
		return;

	UpdateMemory();
	UpdateIdealThreat();
	UpdateThreat();
}

//================================================================================
// Update memory, get information about enemies and friends.
//================================================================================
void CBotMemory::UpdateMemory()
{
	VPROF_BUDGET("CBotMemory::UpdateMemory", VPROF_BUDGETGROUP_BOTS_EXPENSIVE);

	UpdateDataMemory("NearbyThreats", 0);
	UpdateDataMemory("NearbyFriends", 0);
	UpdateDataMemory("NearbyDangerousThreats", 0);

	if (m_DataMemory.Count() > 0) {
		VPROF_BUDGET("DataMemory", VPROF_BUDGETGROUP_BOTS_EXPENSIVE);

		FOR_EACH_MAP_FAST(m_DataMemory, it)
		{
			CDataMemory *memory = m_DataMemory[it];
			Assert(memory);

			if (memory->IsExpired()) {
				m_DataMemory.RemoveAt(it);
				delete memory;
				memory = NULL;
				continue;
			}
		}
	}

	{
		VPROF_BUDGET("EntityMemory", VPROF_BUDGETGROUP_BOTS_EXPENSIVE);

		int nearbyThreats = 0;
		int nearbyFriends = 0;
		int nearbyDangerousThreats = 0;

		for (int it = 0; it < MAX_ENTITY_MEMORY; it++) {
			CEntityMemory *memory = m_Memory[it];

			if (!memory)
				continue;

			CBaseEntity *pEntity = memory->GetEntity();

			// The entity has been deleted.
			if (pEntity == NULL || pEntity->IsMarkedForDeletion() || !pEntity->IsAlive()) {
				ForgetEntity(it);
				continue;
			}

			// New frame, we need to recheck if we can see it.
			memory->UpdateVisibility(false);

			if (!memory->IsLost()) {
				// The last known position of this entity is close to us.
				// We mark how many allied/enemy entities are close to us to make better decisions.
				if (memory->IsInRange(m_flNearbyDistance)) {
					if (memory->IsEnemy()) {
						if (GetDecision()->IsDangerousEnemy(pEntity)) {
							++nearbyDangerousThreats;
						}

						++nearbyThreats;
					}
					else if (memory->IsFriend()) {
						++nearbyFriends;
					}
				}
			}
		}

		UpdateDataMemory("NearbyThreats", nearbyThreats);
		UpdateDataMemory("NearbyFriends", nearbyFriends);
		UpdateDataMemory("NearbyDangerousThreats", nearbyDangerousThreats);
	}

	// We see, we smell, we feel
	GetDecision()->PerformSensing();
}

//================================================================================
// Update who should be our main threat.
//================================================================================
void CBotMemory::UpdateIdealThreat()
{
	VPROF_BUDGET("CBotMemory::UpdateIdealThreat", VPROF_BUDGETGROUP_BOTS_EXPENSIVE);

	CEntityMemory *pIdeal = NULL;

	for (int it = 0; it < MAX_ENTITY_MEMORY; it++) {
		CEntityMemory *memory = m_Memory[it];

		if (!memory)
			continue;

		if (memory->IsLost())
			continue;

		if (!memory->IsEnemy())
			continue;

		if (!pIdeal || GetDecision()->IsBetterEnemy(memory, pIdeal)) {
			pIdeal = memory;
		}
	}

	m_pIdealThreat = pIdeal;
}

//================================================================================
// Update who will be our primary threat.
//================================================================================
void CBotMemory::UpdateThreat()
{
	VPROF_BUDGET("CBotMemory::UpdateThreat", VPROF_BUDGETGROUP_BOTS_EXPENSIVE);

	// We totally lost it
	if (m_pPrimaryThreat && m_pPrimaryThreat->IsLost()) {
		m_pPrimaryThreat = NULL;
	}

	if (!m_pPrimaryThreat) {
		// We do not have a primary threat, 
		// if we have an ideal threat then we use it as the primary,
		// otherwise there is no enemy.
		if (m_pIdealThreat) {
			m_pPrimaryThreat = m_pIdealThreat;
		}
		else {
			return;
		}
	}
	else {
		// We already have a primary threat but the ideal threat is better.
		if (m_pIdealThreat && m_pPrimaryThreat != m_pIdealThreat) {
			if (GetDecision()->IsBetterEnemy(m_pIdealThreat, m_pPrimaryThreat)) {
				m_pPrimaryThreat = m_pIdealThreat;
			}
		}
	}

	// Update the location of hitboxes
	m_pPrimaryThreat->UpdateHitboxAndVisibility();
}

//================================================================================
// It maintains the current primary threat, preventing it from being forgotten.
//================================================================================
void CBotMemory::MaintainThreat()
{
	CEntityMemory *memory = GetPrimaryThreat();

	if (memory != NULL) {
		memory->Maintain();
	}
}

//================================================================================
//================================================================================
float CBotMemory::GetPrimaryThreatDistance() const
{
	CEntityMemory *memory = GetPrimaryThreat();

	if (!memory)
		return -1.0f;

	return memory->GetDistance();
}

//================================================================================
//================================================================================
void CBotMemory::SetEnemy(CBaseEntity * pEnt, bool bUpdate)
{
	if (pEnt == NULL) {
		m_pPrimaryThreat = NULL;
		return;
	}

	if (pEnt->IsMarkedForDeletion())
		return;

	// What?
	if (pEnt == GetHost())
		return;

	if (!GetDecision()->CanBeEnemy(pEnt))
		return;

	CEntityMemory *memory = GetEntityMemory(pEnt);

	if (bUpdate || !memory) {
		memory = UpdateEntityMemory(pEnt, pEnt->WorldSpaceCenter());
	}

	if (m_pPrimaryThreat == memory)
		return;

	m_pPrimaryThreat = memory;
	SetCondition(BCOND_NEW_ENEMY);
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::UpdateEntityMemory(CBaseEntity * pEnt, const Vector & vecPosition, CBaseEntity * pInformer)
{
	VPROF_BUDGET("CBotMemory::UpdateEntityMemory", VPROF_BUDGETGROUP_BOTS_EXPENSIVE);

	if (pEnt == NULL || pEnt->IsMarkedForDeletion())
		return NULL;

	// We do not need to remember ourselves...
	if (pEnt == GetHost())
		return NULL;

	if (!IsEnabled())
		return NULL;

	CEntityMemory *memory = GetEntityMemory(pEnt);

	if (memory) {
		// We avoid updating the memory several times in the same frame.
		if (memory->GetLastUpdateFrame() == gpGlobals->absoluteframetime) {
			return memory;
		}

		// Someone else is informing us about this entity.
		if (pInformer) {
			// We already have vision
			if (memory->IsVisible()) {
				return memory;
			}

			// Optimization: If you are informing us, 
			// we will avoid updating until 1 second after the last report.
			if (memory->IsUpdatedRecently(1.0f)) {
				return memory;
			}
		}
	}

	// I have seen this entity with my own eyes, 
	// we must communicate it to our squad.
	if (!pInformer && GetBot()->GetSquad() && GetDecision()->IsEnemy(pEnt)) {
		GetBot()->GetSquad()->ReportEnemy(GetHost(), pEnt);
	}

	AssertOnce(pEnt->entindex() <= MAX_ENTITY_MEMORY);

	// Memory creation
	if (memory == NULL) {
		memory = new CEntityMemory(GetBot(), pEnt, pInformer);
		m_Memory[pEnt->entindex() % MAX_ENTITY_MEMORY] = memory;
	}

	memory->UpdatePosition(vecPosition);
	memory->SetInformer(pInformer);
	memory->LastFrameUpdate();

	return memory;
}

//================================================================================
// Removes the entity from memory
//================================================================================
void CBotMemory::ForgetEntity(CBaseEntity * pEnt)
{
	ForgetEntity(pEnt->entindex());
}

//================================================================================
// Removes index from memory
// Notes:
// 1. It is the index in the array, not the index of the entity.
// 2. Always use this function to safely remove an entity from memory and its known pointers.
//================================================================================
void CBotMemory::ForgetEntity(int index)
{
	CEntityMemory *memory = m_Memory[index];
	Assert(memory);

	if (memory == NULL)
		return;

	// We check and set null all known pointers

	if (memory == m_pPrimaryThreat) {
		m_pPrimaryThreat = NULL;
	}

	if (memory == m_pIdealThreat) {
		m_pIdealThreat = NULL;
	}

	m_Memory[index] = NULL;

	// Delete pointer ???
	delete memory;
	memory = NULL;
}

//================================================================================
//================================================================================
void CBotMemory::ForgetAllEntities()
{
	Reset();
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetEntityMemory(CBaseEntity * pEnt) const
{
	if (pEnt == NULL) {
		if (GetPrimaryThreat() == NULL)
			return NULL;

		return GetPrimaryThreat();
	}

	Assert(pEnt);

	if (pEnt == NULL)
		return NULL;

	return GetEntityMemory(pEnt->entindex());
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetEntityMemory(int entindex) const
{
	return m_Memory[entindex];
}

//================================================================================
//================================================================================
float CBotMemory::GetMemoryDuration() const
{
	return GetProfile()->GetMemoryDuration();
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetClosestThreat(float *distance) const
{
	float closest = MAX_TRACE_LENGTH;
	CEntityMemory *closestMemory = NULL;

	for (int it = 0; it < MAX_ENTITY_MEMORY; it++) {
		CEntityMemory *memory = m_Memory[it];

		if (!memory)
			continue;

		if (memory->IsLost())
			continue;

		if (!memory->IsEnemy())
			continue;

		float distance = memory->GetDistance();

		if (distance < closest) {
			closest = distance;
			closestMemory = memory;
		}
	}

	if (distance) {
		distance = &closest;
	}

	return closestMemory;
}

//================================================================================
//================================================================================
int CBotMemory::GetThreatCount(float range) const
{
	int count = 0;

	for (int it = 0; it < MAX_ENTITY_MEMORY; it++) {
		CEntityMemory *memory = m_Memory[it];

		if (!memory)
			continue;

		if (memory->IsLost())
			continue;

		if (!memory->IsEnemy())
			continue;

		float distance = memory->GetDistance();

		if (distance > range)
			continue;

		++count;
	}

	return count;
}

//================================================================================
//================================================================================
int CBotMemory::GetThreatCount() const
{
	int count = 0;

	for (int it = 0; it < MAX_ENTITY_MEMORY; it++) {
		CEntityMemory *memory = m_Memory[it];

		if (!memory)
			continue;

		if (memory->IsLost())
			continue;

		if (!memory->IsEnemy())
			continue;

		++count;
	}

	return count;
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetClosestFriend(float *distance) const
{
	float closest = MAX_TRACE_LENGTH;
	CEntityMemory *closestMemory = NULL;

	for (int it = 0; it < MAX_ENTITY_MEMORY; it++) {
		CEntityMemory *memory = m_Memory[it];

		if (!memory)
			continue;

		if (memory->IsLost())
			continue;

		if (!memory->IsFriend())
			continue;

		float distance = memory->GetDistance();

		if (distance < closest) {
			closest = distance;
			closestMemory = memory;
		}
	}

	if (distance) {
		distance = &closest;
	}

	return closestMemory;
}

//================================================================================
//================================================================================
int CBotMemory::GetFriendCount(float range) const
{
	int count = 0;

	for (int it = 0; it < MAX_ENTITY_MEMORY; it++) {
		CEntityMemory *memory = m_Memory[it];

		if (!memory)
			continue;

		if (memory->IsLost())
			continue;

		if (!memory->IsFriend())
			continue;

		float distance = memory->GetDistance();

		if (distance > range)
			continue;

		++count;
	}

	return count;
}

//================================================================================
//================================================================================
int CBotMemory::GetFriendCount() const
{
	int count = 0;

	for (int it = 0; it < MAX_ENTITY_MEMORY; it++) {
		CEntityMemory *memory = m_Memory[it];

		if (!memory)
			continue;

		if (memory->IsLost())
			continue;

		if (!memory->IsFriend())
			continue;

		++count;
	}

	return count;
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetClosestKnown(int teamnum, float *distance) const
{
	float closest = MAX_TRACE_LENGTH;
	CEntityMemory *closestMemory = NULL;

	for (int it = 0; it < MAX_ENTITY_MEMORY; it++) {
		CEntityMemory *memory = m_Memory[it];

		if (!memory)
			continue;

		if (memory->IsLost())
			continue;

		if (memory->GetEntity()->GetTeamNumber() != teamnum)
			continue;

		float distance = memory->GetDistance();

		if (distance < closest) {
			closest = distance;
			closestMemory = memory;
		}
	}

	if (distance) {
		distance = &closest;
	}

	return closestMemory;
}

//================================================================================
//================================================================================
int CBotMemory::GetKnownCount(int teamnum, float range) const
{
	int count = 0;

	for (int it = 0; it < MAX_ENTITY_MEMORY; it++) {
		CEntityMemory *memory = m_Memory[it];

		if (!memory)
			continue;

		if (memory->IsLost())
			continue;

		if (memory->GetEntity()->GetTeamNumber() != teamnum)
			continue;

		float distance = memory->GetDistance();

		if (distance > range)
			continue;

		++count;
	}

	return count;
}

//================================================================================
//================================================================================
int CBotMemory::GetTotalKnownCount() const
{
	return 0;
}

//================================================================================
//================================================================================
float CBotMemory::GetTimeSinceVisible(int teamnum) const
{
	float closest = -1.0f;

	for (int it = 0; it < MAX_ENTITY_MEMORY; it++) {
		CEntityMemory *memory = m_Memory[it];

		if (!memory)
			continue;

		if (memory->IsLost())
			continue;

		if (memory->GetEntity()->GetTeamNumber() != teamnum)
			continue;

		float time = memory->GetTimeLastVisible();

		if (time > closest) {
			closest = time;
		}
	}

	return closest;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory(const char * name, const Vector & value, float forgetTime)
{
	CDataMemory *memory = GetDataMemory(name);

	if (memory) {
		memory->SetVector(value);
	}
	else {
		memory = new CDataMemory(value);
		m_DataMemory.Insert(AllocPooledString(name), memory);
	}

	memory->ForgetIn(forgetTime);
	return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory(const char * name, float value, float forgetTime)
{
	CDataMemory *memory = GetDataMemory(name);

	if (memory) {
		memory->SetFloat(value);
	}
	else {
		memory = new CDataMemory(value);
		m_DataMemory.Insert(AllocPooledString(name), memory);
	}

	memory->ForgetIn(forgetTime);
	return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory(const char * name, int value, float forgetTime)
{
	CDataMemory *memory = GetDataMemory(name);

	if (memory) {
		memory->SetInt(value);
	}
	else {
		memory = new CDataMemory(value);
		m_DataMemory.Insert(AllocPooledString(name), memory);
	}

	memory->ForgetIn(forgetTime);
	return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory(const char * name, const char * value, float forgetTime)
{
	CDataMemory *memory = GetDataMemory(name);

	if (memory) {
		memory->SetString(value);
	}
	else {
		memory = new CDataMemory(value);
		m_DataMemory.Insert(AllocPooledString(name), memory);
	}

	memory->ForgetIn(forgetTime);
	return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory(const char * name, CBaseEntity * value, float forgetTime)
{
	if (value == NULL || value->IsMarkedForDeletion())
		return NULL;

	CDataMemory *memory = GetDataMemory(name);

	if (memory) {
		memory->SetEntity(value);
	}
	else {
		memory = new CDataMemory(value);
		m_DataMemory.Insert(AllocPooledString(name), memory);
	}

	memory->ForgetIn(forgetTime);
	return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::AddDataMemoryList(const char * name, CDataMemory * value, float forgetTime)
{
	CDataMemory *memory = GetDataMemory(name);

	if (!memory) {
		memory = new CDataMemory();
		memory->ForgetIn(forgetTime);

		m_DataMemory.Insert(AllocPooledString(name), memory);
	}

	memory->Add(value);
	return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::RemoveDataMemoryList(const char * name, CDataMemory * value, float forgetTime)
{
	CDataMemory *memory = GetDataMemory(name);

	if (!memory) {
		return NULL;
	}

	memory->Remove(value);
	return memory;
}

//================================================================================
//================================================================================
bool CBotMemory::HasDataMemory(const char * name) const
{
	CDataMemory *memory = GetDataMemory(name);
	return (memory == NULL) ? false : true;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::GetDataMemory(const char * name, bool forceIfNotExists) const
{
	string_t szName = AllocPooledString(name);
	int index = m_DataMemory.Find(szName);

	if (!m_DataMemory.IsValidIndex(index)) {
		if (forceIfNotExists) {
			return new CDataMemory();
		}
		else {
			return NULL;
		}
	}

	return m_DataMemory.Element(index);
}

//================================================================================
//================================================================================
void CBotMemory::ForgetData(const char * name)
{
	string_t szName = AllocPooledString(name);
	m_DataMemory.Remove(szName);
}

//================================================================================
//================================================================================ 
void CBotMemory::ForgetAllData()
{
	m_DataMemory.Purge();
}