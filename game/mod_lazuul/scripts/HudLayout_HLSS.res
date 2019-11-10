#base "HudLayoutBase.res"

"Resource/HudLayout.res"
{
	HudHealthHE
	{
		"fieldName"		"HudHealthHE"
		"xpos"			"4"
		"ypos"			"r44"		//r34
		"tall"			"40"
		"wide"			"100"		//144
		"visible" 		"1"
		"enabled" 		"1"

		"PaintBackgroundType"	"2"

		"digi_xpos" 		"4"		//100
		"digi_ypos" 		"2"		//4
		"icon_xpos" 		"4"		//r0
		"icon_ypos" 		"2"		//r16
		
	}
	
	HudSuitHE
	{
		"fieldName"		"HudSuitHE"
		"xpos"			"4"
		"ypos"			"480"	//r34
		"wide"			"100"
		"tall"  		"40" 
		"visible" 		"1"
		"enabled" 		"1"

		"PaintBackgroundType"	"2"

		"digi_xpos" 		"4"		//100
		"digi_ypos" 		"2"		//4
		"icon_xpos" 		"4"		//0
		"icon_ypos" 		"2"		//r16
	}

	HudAmmoHE	
	{
		"fieldName" "HudAmmoHE"
		"xpos"	"r110"
		"ypos"	"r100"
		"wide"	"100"
		"tall"  "50"
		"visible" "1"
		"enabled" "1"

		"PaintBackgroundType"	"2"

		"digi_xpos" 		"4"
		"digi_ypos" 		"2"
		"icon_xpos" 		"4"
		"icon_ypos" 		"2"
	}

	HudAmmoSecondaryHE	
	{
		"fieldName" "HudAmmoSecondaryHE"
		"xpos"	"r110"
		"ypos"	"r45"
		"wide"	"100"
		"tall"  "40"
		"visible" "1"
		"enabled" "1"

		"PaintBackgroundType"	"2"

		"digi_xpos" 		"4"
		"digi_ypos" 		"2"
		"icon_xpos" 		"2"
		"icon_ypos" 		"2"

	}
	
	HudFlashlight
	{
		"fieldName" "HudFlashlight"
		"visible" "1"
		"PaintBackgroundType"	"2"
		"xpos"	"2"		
		"ypos"	"2"					
		"tall"  "24"
		"wide"	"36"
		"font"	"WeaponIconsSmall"
		
		"icon_xpos"	"4"
		"icon_ypos" "-8"
		
		"BarInsetX" "4"
		"BarInsetY" "18"
		"BarWidth" "28"
		"BarHeight" "2"
		"BarChunkWidth" "2"
		"BarChunkGap" "1"
	}
	
	HudLocator
	{
		"fieldName" "HudLocator"
		"visible" "1"
		"PaintBackgroundType"	"2"
		"xpos"	"c64"	
		"ypos"	"r36"
		"wide"	"64"
		"tall"  "24"
	}
	
	HudWeaponSelection
	{
		"fieldName" "HudWeaponSelection"
		"ypos" 	"0"				//480
		"tall"	"480"
		"visible" "1"
		"enabled" "1"
		"SmallBoxSize" "32"
		"MediumBoxWide"	"95"
		"MediumBoxWide_hidef"	"78"
		"MediumBoxTall"	"50"
		"MediumBoxTall_hidef"	"50"
		"MediumBoxWide_lodef"	"74"
		"MediumBoxTall_lodef"	"50"
		"LargeBoxWide" 		"84"
		"LargeBoxTall"		"54"
		"LargeBoxTallAnim" 	"54"
		"BoxGap" "4"
		"SelectionNumberXPos" "4"
		"SelectionNumberYPos" "4"
		"SelectionGrowTime"	"0.4"
		"TextYPos" "0" //40?
		
		"HLSSMode" "1" // Very Important
	}
	
	HudHistoryResource	
	{
		"fieldName" "HudHistoryResource"
		"visible" "1"
		"enabled" "1"
		"xpos"	"r252"
		"ypos"	"40"
		"wide"	 "248"
		"tall"	 "480"

		"history_gap"	"56"
		"icon_inset"	"38"
		"text_inset"	"36"
		"NumberFont"	"HudNumbersSmall"
	}
	
	HudSquadStatus	
	{
		"fieldName"	"HudSquadStatus"
		"visible"	"1"
		"enabled" "1"
		"xpos"	"146"
		"ypos"	"4"
		"wide"	"104"
		"tall"	"46"
		"text_xpos"	"8"
		"text_ypos"	"34"
		"SquadIconColor"	"SquadFg"
		"IconInsetX"	"8"
		"IconInsetY"	"0"
		"IconGap"		"24"

		"PaintBackgroundType"	"2"
	}
	
	HudPoisonDamageIndicator	
	{
		"fieldName"	"HudPoisonDamageIndicator"
		"visible"	"0"
		"enabled" "1"
		"xpos"	"144"
		"ypos"	"20"
		"wide"	"136"
		"tall"	"38"
		"text_xpos"	"8"
		"text_ypos"	"8"
		"text_ygap" "14"
		"TextColor"	"255 170 0 220"
		"PaintBackgroundType"	"2"
	}
	
	HudSuitPower
	{
		"fieldName" "HudSuitPower"
		"visible" "1"
		"enabled" "1"
		"xpos"	"40"
		"ypos"	"4"
		"wide"	"102"
		"tall"	"26"
		
		"AuxPowerLowColor" "255 0 0 220"
		"AuxPowerHighColor" "BrightFg"
		"AuxPowerDisabledAlpha" "70"

		"BarInsetX" "8"
		"BarInsetY" "15"
		"BarWidth" "92"
		"BarHeight" "4"
		"BarChunkWidth" "6"
		"BarChunkGap" "3"

		"text_xpos" "8"
		"text_ypos" "4"
		"text2_xpos" "8"
		"text2_ypos" "22"
		"text2_gap" "10"

		"PaintBackgroundType"	"2"
	}
}