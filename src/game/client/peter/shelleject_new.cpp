#include "cbase.h"
#include "shelleject_new.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "physpropclientside.h"

bool CShellEjectScriptSystem::Init()
{
	FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;
	const char *fileName = "scripts/animevents/*.txt";
	char szFullFileName[MAX_PATH];
	fileName = g_pFullFileSystem->FindFirstEx(fileName, "GAME", &findHandle);
	while (fileName)
	{
		char name[32];
		V_StripExtension(fileName, name, 32);
		Q_snprintf(szFullFileName, sizeof(szFullFileName), "scripts/animevents/%s", fileName);
		KeyValues *pKVFile = new KeyValues(name);
		if (pKVFile->LoadFromFile(filesystem, szFullFileName))
		{
			shellMap_t *script = m_Scripts[name] = new shellMap_t;
			for (KeyValues * pkvEvent = pKVFile->GetFirstTrueSubKey(); pkvEvent != NULL; pkvEvent = pkvEvent->GetNextTrueSubKey())
			{
				scriptShell_t shell;
				shell.iCount = pkvEvent->GetInt("casing_count", 1);
				shell.forward_speed_min = pkvEvent->GetFloat("forward_speed_min", 100.0f);
				shell.forward_speed_max = pkvEvent->GetFloat("forward_speed_max", 150.0f);
				shell.right_speed_max = pkvEvent->GetFloat("right_speed_max", -10.0f);
				shell.right_speed_min = pkvEvent->GetFloat("right_speed_min", 10.0f);
				shell.up_speed_max = pkvEvent->GetFloat("up_speed_max", 0.0f);
				shell.up_speed_min = pkvEvent->GetFloat("up_speed_min", 10.0f);
				shell.lifetime = pkvEvent->GetFloat("lifetime", 10.0f);
				shell.iSkin = pkvEvent->GetInt("skin");
				shell.flGravityScale = pkvEvent->GetFloat("gravity_scale", 1.0f);
				V_strncpy(shell.cModelName, pkvEvent->GetString("eject_model", "models/weapons/shell.mdl"), MAX_PATH);

				script->Insert(pkvEvent->GetName(), shell);
			}
		}

		pKVFile->deleteThis();
		fileName = g_pFullFileSystem->FindNext(findHandle);
	}

	return true;
}

void CShellEjectScriptSystem::Shutdown()
{
	m_Scripts.PurgeAndDeleteElements();
}

void CShellEjectScriptSystem::LevelInitPreEntity()
{
	for (int i = 0; i < m_Scripts.GetNumStrings(); i++)
	{
		shellMap_t *shellMap = m_Scripts[i];
		for (unsigned int j = 0; j < shellMap->Count(); j++)
		{
			scriptShell_t *pShellEffect = &shellMap->Element(j);
			pShellEffect->iModelIndex = CBaseEntity::PrecacheModel(pShellEffect->cModelName);
		}
	}
}

// Options format: (weapon name) (shell effect) (attachment)
// If used on a viewmodel or weapon, uses the name of the weapon
// If econ items are enabled, uses the item for lookups
void CShellEjectScriptSystem::EjectShell(C_BaseAnimating * pEnt, const char * options)
{
	char token[256];
	char szWeaponClass[MAX_WEAPON_STRING];

	C_BaseCombatWeapon *pWeapon = nullptr;
	C_EconEntity *pEcon = nullptr;
	if (pEnt->IsViewModel())
	{
		C_BaseViewModel *pVM = static_cast<C_BaseViewModel *> (pEnt);
		pWeapon = pVM->GetWeapon();
		pEcon = pWeapon;
	}
	else if (pEnt->IsBaseCombatWeapon())
	{
		pWeapon = static_cast<C_BaseCombatWeapon *> (pEnt);
		pEcon = pWeapon;
	}
	else
	{
		pEcon = dynamic_cast<C_EconEntity *> (pEnt);
	}

	const char *p = options;

	p = nexttoken(token, p, ' ');
	const char *pszWeapName = token;
	if (pWeapon)
	{
		pszWeapName = pWeapon->GetWpnData().szClassName;
	}

#ifdef USES_ECON_ITEMS
	if (pEcon)
	{
		CEconItemDefinition* pItemDef = pEcon->GetItem()->GetStaticData();
		if (pItemDef)
		{
			if (pItemDef->shell_script[0])
				pszWeapName = pItemDef->shell_script;
		}
	}
#endif
	
	Q_strncpy(szWeaponClass, pszWeapName, sizeof(szWeaponClass));

	UtlSymId_t IDWeapon = m_Scripts.Find(szWeaponClass);
	if (IDWeapon == m_Scripts.InvalidIndex())
	{
		Warning("EjectShellNew: No script file for \'%s\'!\n", szWeaponClass);
		return;
	}

	shellMap_t *shellMap = m_Scripts[IDWeapon];

	// Get the desired effect
	p = nexttoken(token, p, ' ');

	UtlSymId_t IDShell = shellMap->Find(token);
	if (!shellMap->IsValidIndex(IDShell))
	{
		Warning("EjectShellNew: No shell effect \'%s\' in file \'%s\'!\n", token, szWeaponClass);
		return;
	}

	scriptShell_t *pShellEffect = &shellMap->Element(IDShell);

	// Get the desired attachment
	p = nexttoken(token, p, ' ');

	Vector vecPos, vecUp, vecRight, vecForward;
	QAngle angAttachment;
	pEnt->GetAttachment(token, vecPos, angAttachment);
	AngleVectors(angAttachment, &vecForward, &vecRight, &vecUp);

	C_PhysPropClientside *pShellProp = C_PhysPropClientside::CreateNew();

	if (!pShellProp)
		return;

	/*const model_t *model = modelinfo->GetModel(pShellEffect->iModelIndex);

	if (!model)
	{
		DevMsg("CTempEnts::PhysicsProp: model index %i not found\n", pShellEffect->iModelIndex);
		return;
	}*/

	pShellProp->SetModelName(pShellEffect->cModelName);
	pShellProp->m_nSkin = pShellEffect->iSkin;
	pShellProp->SetAbsOrigin(vecPos);
	pShellProp->SetAbsAngles(pEnt->GetAbsAngles());
	pShellProp->SetPhysicsMode(PHYSICS_MULTIPLAYER_CLIENTSIDE);
	pShellProp->SetEffects(EF_NOSHADOW);
	pShellProp->SetGravity(pShellEffect->flGravityScale);

	if (!pShellProp->Initialize())
	{
		pShellProp->Release();
		return;
	}

	IPhysicsObject *pPhysicsObject = pShellProp->VPhysicsGetObject();

	if (pPhysicsObject)
	{
		Vector vel = RandomVector(-2.f, 2.f);
		vel += vecForward * RandomFloat(pShellEffect->forward_speed_min, pShellEffect->forward_speed_max);
		vel += vecUp * RandomFloat(pShellEffect->up_speed_min, pShellEffect->up_speed_max);
		vel += vecRight * RandomFloat(pShellEffect->right_speed_min, pShellEffect->right_speed_max);
		pPhysicsObject->AddVelocity(&vel, NULL);
	}
	else
	{
		// failed to create a physics object
		pShellProp->Release();
		return;
	}

	pShellProp->StartFadeOut(pShellEffect->lifetime);
}

CShellEjectScriptSystem g_ShellSystem;

void CC_ShellEject_Reload_f()
{
	g_ShellSystem.Shutdown();
	g_ShellSystem.Init();
	if (engine->IsInGame())
		g_ShellSystem.LevelInitPreEntity();
}

ConCommand cc_shelleject_reload("cl_shelleject_reload", CC_ShellEject_Reload_f);

CShellEjectScriptSystem * NewShellSystem()
{
	return &g_ShellSystem;
}
