"GameMenu"
{
	"1"
	{
		"label" "#GameUI_GameMenu_ResumeGame"
		"command" "ResumeGame"
		"InGameOrder" "10"
		"OnlyInGame" "1"
	}
	"2"
	{
		"label" "#GameUI_GameMenu_NewGame"
		"command" "OpenNewGameDialog"
		//"command"	"engine hlms_newgame"
		"InGameOrder" "50"
		"notmulti" "1"
	}
	"3"
	{
		"label" "#GameUI_GameMenu_PlayerList"
		"command" "OpenPlayerListDialog"
		"OnlyInGame" "1"
		//"InGameOrder"	"30"
		"notsingle"	"1"
	}
	"4"
	{
		"label" ""
		"command" ""
		"OnlyInGame" "1"
		"InGameOrder" "40"
	}
	"5"
	{
		"label" "#GameUI_GameMenu_LoadGame"
		"command" "OpenLoadGameDialog"
		//"command" "engine ent_fire load setanimation podExtractor_extract"
		"InGameOrder" "30"
		"notmulti" "1"
	}
	"6"
	{
		"label" "#GameUI_GameMenu_SaveGame"
		"command" "OpenSaveGameDialog"
		"InGameOrder" "20"
		"notmulti" "1"
		"OnlyInGame" "1"
	}
	"7"
	{
		"label"	"#GameUI_LoadCommentary"
		"command" "OpenLoadSingleplayerCommentaryDialog"
		//"command"	"engine hlms_newgame commentary"
		"InGameOrder" "60"
		"notmulti" "1"
	}
	"8"	[$WIN32]
	{
		"label" "#GameUI_GameMenu_BonusMaps"
		"command" "OpenBonusMapsDialog"
		"InGameOrder" "70"
		"notmulti" "1"
	}
	"9"
	{
		"label" "#GameUI_GameMenu_Achievements"
		"command" "OpenAchievementsDialog"
		"InGameOrder" "80"
	}
	"10"
	{
		"label" "#GameUI_GameMenu_FindServers"
		"command" "OpenServerBrowser"
		"InGameOrder" "50"
		"notsingle"	"1"
	}
	"11"
	{
		"label" "#GameUI_GameMenu_CreateServer"
		"command" "OpenCreateMultiplayerGameDialog"
		"InGameOrder" "60"
		"notsingle"	"1"
	}
	"12"
	{
		"label" "#GameUI_GameMenu_Options"
		"command" "OpenOptionsDialog"
		//"command" "engine ent_fire settings sparkonce"
		"InGameOrder" "90"
	}
	// "10"
	// {
		// "label" "#GameUI_GameMenu_C17Options"
		// "command" "engine OpenModOptionsDialog"
		// "InGameOrder" "100"
	// }
	"13"
	{
		"label" "#GameUI_GameMenu_MainMenu"
		"command" "disconnect"
		"OnlyInGame" "1"
		"InGameOrder" "110"
	}
	"14"
	{
		"label" "#GameUI_GameMenu_Quit"
		"command" "Quit"
		"InGameOrder" "120"
	}
}

