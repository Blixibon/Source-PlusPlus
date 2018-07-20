#ifdef BotPlayerClass
//================================================================================
//================================================================================
void BotPlayerClass::SetBotController(IBot * pBot)
{
	if (m_pBotController) {
		delete m_pBotController;
		m_pBotController = NULL;
	}

	m_pBotController = pBot;
}

//================================================================================
//================================================================================
void BotPlayerClass::SetUpBot()
{
	CreateSenses();
	SetBotController(new CBot(this));
}

//================================================================================
//================================================================================
void BotPlayerClass::CreateSenses()
{
	m_pSenses = new CAI_Senses;
	m_pSenses->SetOuter(this);
}

//================================================================================
//================================================================================
void BotPlayerClass::SetDistLook(float flDistLook)
{
	if (GetSenses()) {
		GetSenses()->SetDistLook(flDistLook);
	}
}

//================================================================================
//================================================================================
int BotPlayerClass::GetSoundInterests()
{
	return SOUND_DANGER | SOUND_COMBAT | SOUND_PLAYER | SOUND_CARCASS | SOUND_MEAT | SOUND_GARBAGE;
}

//================================================================================
//================================================================================
int BotPlayerClass::GetSoundPriority(CSound *pSound)
{
	if (pSound->IsSoundType(SOUND_COMBAT)) {
		return SOUND_PRIORITY_HIGH;
	}

	if (pSound->IsSoundType(SOUND_DANGER)) {
		if (pSound->IsSoundType(SOUND_CONTEXT_FROM_SNIPER | SOUND_CONTEXT_EXPLOSION)) {
			return SOUND_PRIORITY_HIGHEST;
		}
		else if (pSound->IsSoundType(SOUND_CONTEXT_GUNFIRE | SOUND_BULLET_IMPACT)) {
			return SOUND_PRIORITY_VERY_HIGH;
		}

		return SOUND_PRIORITY_HIGH;
	}

	if (pSound->IsSoundType(SOUND_CARCASS | SOUND_MEAT | SOUND_GARBAGE)) {
		return SOUND_PRIORITY_VERY_LOW;
	}

	return SOUND_PRIORITY_NORMAL;
}

//================================================================================
//================================================================================
bool BotPlayerClass::QueryHearSound(CSound *pSound)
{
	CBaseEntity *pOwner = pSound->m_hOwner.Get();

	if (pOwner == this)
		return false;

	if (pSound->IsSoundType(SOUND_PLAYER) && !pOwner) {
		return false;
	}

	if (pSound->IsSoundType(SOUND_CONTEXT_ALLIES_ONLY)) {
		if (Classify() != CLASS_PLAYER_ALLY && Classify() != CLASS_PLAYER_ALLY_VITAL) {
			return false;
		}
	}

	if (pOwner) {
		// Solo escuchemos sonidos provocados por nuestros aliados si son de combate.
		if (TheGameRules->PlayerRelationship(this, pOwner) == GR_ALLY || TheGameRules->PlayerRelationship(this, pOwner) == GR_TEAMMATE) {
			if (pSound->IsSoundType(SOUND_COMBAT) && !pSound->IsSoundType(SOUND_CONTEXT_GUNFIRE)) {
				return true;
			}

			return false;
		}
	}

	if (ShouldIgnoreSound(pSound)) {
		return false;
	}

	return true;
}

//================================================================================
//================================================================================
bool BotPlayerClass::QuerySeeEntity(CBaseEntity *pEntity, bool bOnlyHateOrFear)
{
	if (bOnlyHateOrFear) {
		if (g_pGameRules->PlayerRelationship(this, pEntity) == GR_NOTTEAMMATE || g_pGameRules->PlayerRelationship(this, pEntity) == GR_ENEMY)
			return true;

		Disposition_t disposition = IRelationType(pEntity);
		return (disposition == D_HT || disposition == D_FR);
	}

	return true;
}

//================================================================================
//================================================================================
void BotPlayerClass::OnLooked(int iDistance)
{
	if (GetBotController()) {
		GetBotController()->OnLooked(iDistance);
	}
}

//================================================================================
//================================================================================
void BotPlayerClass::OnListened()
{
	if (GetBotController()) {
		GetBotController()->OnListened();
	}
}

//================================================================================
//================================================================================
CSound *BotPlayerClass::GetLoudestSoundOfType(int iType)
{
	return CSoundEnt::GetLoudestSoundOfType(iType, EarPosition());
}

//================================================================================
// Devuelve si podemos ver el origen del sonido
//================================================================================
bool BotPlayerClass::SoundIsVisible(CSound *pSound)
{
	return (FVisible(pSound->GetSoundReactOrigin()) && IsInFieldOfView(pSound->GetSoundReactOrigin()));
}

//================================================================================
//================================================================================
CSound* BotPlayerClass::GetBestSound(int validTypes)
{
	CSound *pResult = GetSenses()->GetClosestSound(false, validTypes);

	if (pResult == NULL) {
		DevMsg("NULL Return from GetBestSound\n");
	}

	return pResult;
}

//================================================================================
//================================================================================
CSound* BotPlayerClass::GetBestScent()
{
	CSound *pResult = GetSenses()->GetClosestSound(true);

	if (pResult == NULL) {
		DevMsg("NULL Return from GetBestScent\n");
	}

	return pResult;
}
#else
#error BotPlayerClass is undefined!
#endif