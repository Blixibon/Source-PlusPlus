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
		"InGameOrder" "40"
		"notmulti" "1"
	}
	"3"	[$WIN32]
	{
		"label" "#GameUI_GameMenu_BonusMaps"
		"command" "OpenBonusMapsDialog"
		"InGameOrder" "50"
		"notmulti" "1"
	}
	"4"
	{
		"label" "#GameUI_GameMenu_LoadGame"
		"command" "OpenLoadGameDialog"
		//"command" "engine ent_fire load setanimation podExtractor_extract"
		"InGameOrder" "30"
		"notmulti" "1"
	}
	"5"
	{
		"label" "#GameUI_GameMenu_SaveGame"
		"command" "OpenSaveGameDialog"
		"InGameOrder" "20"
		"notmulti" "1"
		"OnlyInGame" "1"
	}
	"6"
	{
		"label"	"#GameUI_LoadCommentary"
		"command" "OpenLoadSingleplayerCommentaryDialog"
		//"command"	"engine hlms_newgame commentary"
		"InGameOrder" "60"
		"notmulti" "1"
	}
	"7"
	{
		"label" "#GameUI_GameMenu_Achievements"
		"command" "OpenAchievementsDialog"
		"InGameOrder" "70"
	}
	"8"
	{
		"label" "#GameUI_Controller"
		"command" "OpenControllerDialog"
		"InGameOrder" "80"
		"ConsoleOnly" "1"
	}
	"9"
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
	"11"
	{
		"label" "#GameUI_GameMenu_MainMenu"
		"command" "disconnect"
		"OnlyInGame" "1"
		"InGameOrder" "110"
	}
	"12"
	{
		"label" "#GameUI_GameMenu_Quit"
		"command" "Quit"
		"InGameOrder" "120"
	}
}

