#base "ClientSchemeBase.res"

Scheme
{
    "Colors"
    {
        "HUDBlueTeam"		"104 124 155 127"
		"HUDRedTeam"		"180 92 77 127"
		"HUDSpectator"		"124 124 124 127"
		"HUDBlueTeamSolid"	"104 124 155 255"
		"HUDRedTeamSolid"	"180 92 77 255"
		"HUDDeathWarning"	"255 0 0 255"
		"HudWhite"			"255 255 255 255"
		"HudOffWhite"		"200 187 161 255"
		"HudBlack"			"65 65 65 255"
		"ProgressBarBlue"	"91 122 142 255"
    }

	"BaseSettings"
	{
		"BrightBg"		"250 220 0 80"
		
		"SquadFg"		"255 220 0 160"
		
		// VXP: Old HL2 HUD
	//	"OldFgColor"			"177 150 70 255"
		"OldFgColor"			"247 192 84 255"
	//	"OldBrightFg"			"224 188 89 255"
		"OldBrightFg"			"244 209 144 255"
		"OldDamagedFg"			"180 0 0 255"
		"OldBrightDamagedFg"	"BrightDamagedFg"
	
		// Top-left corner of the "Half-Life 2" on the main screen
		"Main.Title1.X"				"86"
		"Main.Title1.Y"				"145"
		"Main.Title1.Y_hidef"		"130"
		"Mod.Main.Title1.Color"	"255 255 255 255"

		// Top-left corner of secondary title e.g. "DEMO" on the main screen
		"Main.Title2.X"				"32"
		"Main.Title2.Y"				"190"
		"Main.Title2.Y_hidef"		"174"
		"Mod.Main.Title2.Color"	"255 255 255 200"		
	}
	
	Fonts
	{
		HudTimerFont
		{
			"1"
			{
				"name"		"Verdana"
				"tall"		"28"
				"tall_hidef"	"25"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"range"		"0x0000 0x017F" //	Basic Latin, Latin-1 Supplement, Latin Extended-A
			}
		}
		HudTimerFontGlow
		{
			"1"
			{
				"name"		"Verdana"
				"tall"		"28"
				"tall_hidef"	"25"
				"weight"	"0"
				"antialias" 	"1"
				"blur"		"5"
				"scanlines"	"2"
				"additive"	"1"
				"range"		"0x0000 0x017F" //	Basic Latin, Latin-1 Supplement, Latin Extended-A
			}
		}
	}
}