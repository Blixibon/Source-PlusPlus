"Resource/HudLayout.res"
{
	HudHealth
	{
		"fieldName"		"HudHealth"
		"xpos"	"16"
		"ypos"	"432"
		"wide"	"102"
		"tall"  "36"
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"2"
		
		"text_xpos" "8"
		"text_ypos" "20"
		"digit_xpos" "50"
		"digit_ypos" "2"
	}
	
	HudHealthOld
	{
		"fieldName"		"HudHealthOld"
	//	"xpos"	"16"
		"xpos"	"13.5"
	//	"ypos"	"432"
	//	"ypos"	"443"
		"ypos"	"445"
		"wide"	"102"
	//	"tall"  "36"
		"tall"  "27"
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"2"
	//	"Color" "247 192 84 255"
	//	"Color" "OldFgColor"
		
		"text_xpos" "0"
		"text_ypos" "1"
	//	"text_width" "29"
	//	"text_height" "9"
		"text_width" "31"
		"text_height" "8"
	//	"digit_xpos" "6"
	//	"digit_xpos" "36"
		"digit_xpos" "38"
		"digit_ypos" "9"
	//	"digit_height" "18"
		"digit_height" "17" // VXP: Good!
		
		// Almost:	health_label	-	"x"			"163"
		//								"y"			"30"
		//								"width"		"46"
		//								"height"	"10"
		//			HudHealth2		-	"text_width" "29"
		//								"text_height" "6"
		
		"NumberFont"		"HudNumbers2"
		"NumberGlowFont"	"HudNumbersGlow2"
		"TextFont"			"Default2"
	}
	
	HudSuit
	{
		"fieldName"		"HudSuit"
		"xpos"	"140"
		"ypos"	"432"
		"wide"	"108"
		"tall"  "36"
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"2"

		"text_xpos" "8"
		"text_ypos" "20"
		"digit_xpos" "50"
		"digit_ypos" "2"
	}
	
	HudSuitOld
	{
		"fieldName"		"HudSuitOld"
	//	"xpos"	"90"
	//	"xpos"	"79"
		"xpos"	"80"
	//	"ypos"	"432"
	//	"ypos"	"443"
		"ypos"	"445"
	//	"wide"	"108"
		"wide"	"102"
	//	"tall"  "36"
		"tall"  "27"
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"2"

	//	"Color" "177 150 70 255"
	//	"Color" "247 192 84 255"
	//	"Color" "OldFgColor"
		
		"text_xpos" "0"
		"text_ypos" "1"
	//	"text_width" "25"
		"text_width" "24.8"
		"text_height" "8"
	//	"digit_xpos" "6"
	//	"digit_xpos" "36"
		"digit_xpos" "38"
		"digit_ypos" "9"
		"digit_height" "17"
		
		"NumberFont"		"HudNumbers2"
		"NumberGlowFont"	"HudNumbersGlow2"
		"TextFont"			"Default2"
	}

	HudAmmo
	{
		"fieldName" "HudAmmo"
		"xpos"	"r150"
		"ypos"	"432"
		"wide"	"136"
		"tall"  "36"
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"2"

		"text_xpos" "8"
		"text_ypos" "20"
		"digit_xpos" "44"
		"digit_ypos" "2"
		"digit2_xpos" "98"
		"digit2_ypos" "16"
	}
	
	HudAmmoOld
	{
		"fieldName" "HudAmmoOld"
		"xpos"	"r20"
	//	"ypos"	"432"
	//	"ypos"	"443"
		"ypos"	"445"
	//	"wide"	"100"
		"wide"	"102"
	//	"tall"  "30"
	//	"tall"  "36"
		"tall"  "27"
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"2"
	//	"Color" "177 150 70 255"
	//	"Color" "247 192 84 255"
	//	"Color" "OldFgColor"

		"text_xpos" "0"
		"text_ypos" "1"
	//	"text_width" "27"
	//	"text_width" "24"
		"text_width" "25"
	//	"text_height" "9"
		"text_height" "8"
	//	"digit_xpos" "6"
	//	"digit_xpos" "50"
	//	"digit_xpos" "36"
		"digit_xpos" "38"
		"digit_ypos" "9"
	//	"digit_height" "18"
		"digit_height" "17"
	//	"digit2_xpos" "70"
	//	"digit2_xpos" "75"
	//	"digit2_xpos" "80" // digit_xpos + 30
	//	"digit2_xpos" "66"
	//	"digit2_xpos" "63"
	//	"digit2_xpos" "62"
	//	"digit2_xpos" "66"
		"digit2_xpos" "64"
	//	"digit2_ypos" "4"
		"digit2_ypos" "13"
		"digit2_height" "9"
		
		"NumberFont"		"HudNumbers2"
		"NumberGlowFont"	"HudNumbersGlow2"
		"TextFont"			"Default2"
	}

	HudAmmoSecondary
	{
		"fieldName" "HudAmmoSecondary"
		"xpos"	"r76"
		"ypos"	"432"
		"wide"	"60"
		"tall"  "36"
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"2"

		"text_xpos" "8"
		"text_ypos" "22"
		"digit_xpos" "36"
		"digit_ypos" "2"
	}
	
	HudAmmoSecondaryOld
	{
		"fieldName" "HudAmmoSecondaryOld"
	//	"xpos"	"r53"
		"xpos"	"r27"
	//	"ypos"	"432"
	//	"ypos"	"443"
	//	"ypos"	"445"
		"ypos"	"454"
	//	"wide"	"60"
	//	"wide"	"80"
		"wide"	"17"
	//	"tall"  "30"
	//	"tall"  "27"
		"tall"  "16"
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"0"
	//	"Color" "177 150 70 255"
	//	"Color" "OldFgColor"

	//	"digit_xpos" "10"
	//	"digit_xpos" "50"
	//	"digit_xpos" "35"
	//	"digit_ypos" "13"

		// VXP: WTF? Why this offset?
		"digit_xpos" "15"
	//	"digit_ypos" "4"
		"digit_ypos" "4"

	//	"digit_xpos" "1"
	//	"digit_ypos" "1"
		"digit_height" "9"
		
		"NumberFont"		"HudNumbers2"
		"NumberGlowFont"	"HudNumbersGlow2"
		"TextFont"			"Default2"
	}

	HudVoiceSelfStatus
	{
		"fieldName" "HudVoiceSelfStatus"
		"visible" "1"
		"enabled" "1"
		"xpos" "r43"
		"ypos" "355"
		"wide" "24"
		"tall" "24"
	}

	HudVoiceStatus
	{
		"fieldName" "HudVoiceStatus"
		"visible" "1"
		"enabled" "1"
		"xpos" "r145"
		"ypos" "0"
		"wide" "145"
		"tall" "400"

		"item_wide"	"135"
		
		"show_avatar"		"0"
		
		"show_dead_icon"	"1"
		"dead_xpos"			"1"
		"dead_ypos"			"0"
		"dead_wide"			"16"
		"dead_tall"			"16"
		
		"show_voice_icon"	"1"
		"icon_ypos"			"0"
		"icon_xpos"			"15"
		"icon_tall"			"16"
		"icon_wide"			"16"
		
		"text_xpos"			"33"
	}

	HudSuitPower
	{
		"fieldName" "HudSuitPower"
		"visible" "0"
		"enabled" "0"
		"xpos"	"16"
		"ypos"	"396"
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
	
	HudSuitPowerOld
	{
		"fieldName" "HudSuitPowerOld"
		"visible" "0"
		"enabled" "0"
	//	"xpos"	"16"
	//	"xpos"	"187" // 79 + 108 (HudSuit2's xpos + wide)
		"xpos"	"145"
	//	"ypos"	"396"
		"ypos"	"454" // 445 + 9 (HudSuit2's ypos + text_height)
	//	"wide"	"102"
	//	"tall"	"30"
	//	"wide"	"80"
		"wide"	"70"
		"tall"	"12"
		
		"AuxPowerLowColor" "255 0 0 220"
	//	"AuxPowerHighColor" "255 220 0 220"
		"AuxPowerHighColor" "177 150 70 255"
	//	"AuxPowerDisabledAlpha" "70"
		"AuxPowerDisabledAlpha" "255"

	//	"Color" "177 150 70 255" // No use of this

	//	"BarInsetX" "8"
	//	"BarInsetY" "8"
	//	"BarWidth" "92"
	//	"BarHeight" "4"
		"BarInsetX" "0"
		"BarInsetY" "0"
		"BarWidth" "70" // Same as wide
		"BarHeight" "12" // Same as tall

		"PaintBackgroundType"	"2"
	}
	
	HudHealthHE
	{
		"fieldName"		"HudHealthHE"
		"xpos"			"4"
		"ypos"			"r44"		//r34
		"tall"			"40"
		"wide"			"100"		//144
		"visible" 		"0"
		"enabled" 		"0"

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
		"visible" 		"0"
		"enabled" 		"0"

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
		"visible" "0"
		"enabled" "0"

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
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"2"

		"digi_xpos" 		"4"
		"digi_ypos" 		"2"
		"icon_xpos" 		"2"
		"icon_ypos" 		"2"

	}

	HudAR2AltFire	
	{
		"fieldName" "HudAR2AltFire"
		"xpos"	"r90"
		"ypos"	"5"
		"wide"	"80"
		"tall"  "40"
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"2"

		"digi_xpos" 		"4"
		"digi_ypos" 		"2"
		"icon_xpos" 		"2"
		"icon_ypos" 		"2"
	}


	HudAirDefense	
	{
		"fieldName" "HudAirDefense"
		"xpos"	"r90"
		"ypos"	"50"
		"wide"	"80"
		"tall"  "42"
		"visible" "1"
		"enabled" "1"

		"PaintBackgroundType"	"2"

		"recharge_width"	"72"
		"recharge_height"	"5"
		"recharge_gap"		"2"
		"label_xpos" 		"4"
		"label_ypos" 		"4"
	}
	
	HudManhackHealth
	{
		"fieldName" "HudManhackHealth"
		"xpos"	"r150"
		"ypos"	"404"
		"wide"	"136"
		"tall"  "64"
		"visible" "0"
		"enabled" "0"

		"PaintBackgroundType"	"2"

		"text_xpos" "8"
		"text_ypos" "20"
		"digit_xpos" "94"
		"digit_ypos" "2"
		"text2_xpos" "8"
		"text2_ypos" "38"
		"digit2_xpos" "94"
		"digit2_ypos" "38"
	}

	HudRadio	
	{
		"fieldName" "HudRadio"
		"xpos"	"r272"
		"ypos"	"r500"
		"wide"	"154"
		"tall"  "20"
		"visible" "1"
		"enabled" "1"

		"PaintBackgroundType"	"2"

		"visualizer_xpos" 	"4"
		"visualizer_ypos" 	"2"
		"visualizer_width"	"92"
		"visualizer_tall"	"16"
		"base_width"		"100"
		"label_xpos" 		"102"
		"label_ypos" 		"2"
	}
	
	HudPosture	[$WIN32]
	{
		"fieldName" 		"HudPosture"
		"visible" 		"1"
		"PaintBackgroundType"	"2"
		"xpos"	"16"
		"ypos"	"316"
		"tall"  "36"
		"wide"	"36"
		"font"	"WeaponIconsSmall"
		"icon_xpos"	"10"
		"icon_ypos" 	"0"
	}
	HudPosture	[$X360]
	{
		"fieldName" 		"HudPosture"
		"visible" 		"1"
		"PaintBackgroundType"	"2"
		"xpos"	"48"
		"ypos"	"316"
		"tall"  "36"
		"wide"	"36"
		"font"	"WeaponIconsSmall"
		"icon_xpos"	"10"
		"icon_ypos" 	"2"
	}
	
	HudFlashlight
	{
		"fieldName" "HudFlashlight"
		"visible" "0"
		"enabled"	"0"
		"PaintBackgroundType"	"2"
		"xpos"	"270"		[$WIN32]
		"ypos"	"444"		[$WIN32]
		"xpos_hidef"	"306"		[$X360]		// aligned to left
		"xpos_lodef"	"c-18"		[$X360]		// centered in screen
		"ypos"	"428"		[$X360]				
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
		"xpos"	"c8"	[$WIN32]
		"ypos"	"r36"	[$WIN32]
		"xpos"	"c-32"	[$X360]
		"ypos_hidef"	"r52"	[$X360]
		"ypos_lodef"	"r95"	[$X360]		// 52 is aligned to bottom of HudSuit
		"wide"	"64"
		"tall"  "24"
	}

	HudDamageIndicator
	{
		"fieldName" "HudDamageIndicator"
		"visible" "0"
		"enabled" "0"
		"DmgColorLeft" "255 0 0 0"
		"DmgColorRight" "255 0 0 0"
		
		"dmg_xpos" "30"
		"dmg_ypos" "100"
		"dmg_wide" "36"
		"dmg_tall1" "240"
		"dmg_tall2" "200"
	}

	HudZoom
	{
		"fieldName" "HudZoom"
		"visible" "1"
		"enabled" "1"
		"Circle1Radius" "66"
		"Circle2Radius"	"74"
		"DashGap"	"16"
		"DashHeight" "4"	[$WIN32]
		"DashHeight" "6"	[$X360]		
		"BorderThickness" "88"
	}

	HudWeaponSelection
	{
		"fieldName" "HudWeaponSelection"
		"ypos" 	"16"	[$WIN32]
		"ypos" 	"32"	[$X360]
		"visible" "1"
		"enabled" "1"
		"SmallBoxSize" "32"
		"MediumBoxWide"	"95"
		"MediumBoxWide_hidef"	"78"
		"MediumBoxTall"	"50"
		"MediumBoxTall_hidef"	"50"
		"MediumBoxWide_lodef"	"74"
		"MediumBoxTall_lodef"	"50"
		"LargeBoxWide" "112"
		"LargeBoxTall" "80"
		"BoxGap" "8"
		"SelectionNumberXPos" "4"
		"SelectionNumberYPos" "4"
		"SelectionGrowTime"	"0.4"
		"TextYPos" "64"
		"HLSSMode" "0"
	}

	HudCrosshair
	{
		"fieldName" "HudCrosshair"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudVehicle
	{
		"fieldName" "HudVehicle"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	ScorePanel
	{
		"fieldName" "ScorePanel"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudTrain
	{
		"fieldName" "HudTrain"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudMOTD
	{
		"fieldName" "HudMOTD"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudMessage
	{
		"fieldName" "HudMessage"
		"visible" "1"
		"enabled" "1"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	"p1"
		"tall"	"p1"
	}

	HudMenu
	{
		"fieldName" "HudMenu"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
		// SYNERGY
		"OpenCloseTime"	"1"
		"Blur"			"0"
		"TextScane"		"1"
		"TextFont"			"Default"	//	"MenuTextFont"
		"ItemFont"			"Default"	//	"MenuItemFont"
		"ItemFontPulsing"	"Default"	//	"MenuItemFontPulsing"
		"MenuColor"		"MenuColor"
		"MenuItemColor"	"ItemColor"
		"MenuBoxColor"	"MenuBoxBg"
	}

	HudCloseCaption
	{
		"fieldName" "HudCloseCaption"
		"visible"	"0"
		"enabled"	"1"
		"xpos"		"c-250"
		"ypos"		"276"	[$WIN32]
		"ypos_hidef"	"236"	[$X360]
		"ypos_lodef"	"206"	[$X360]			//236
		"wide"		"500"
		"tall"		"136"	[$WIN32]
		"tall"		"176"	[$X360]

		"BgAlpha"	"128"

		"GrowTime"		"0.25"
		"ItemHiddenTime"	"0.2"  // Nearly same as grow time so that the item doesn't start to show until growth is finished
		"ItemFadeInTime"	"0.15"	// Once ItemHiddenTime is finished, takes this much longer to fade in
		"ItemFadeOutTime"	"0.3"
		"topoffset"		"0"		[$WIN32]
		"topoffset"		"40"	[$X360]
	}

	HudChat
	{
		"fieldName" "HudChat"
		"visible" "0"
		"enabled" "1"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	 "4"
		"tall"	 "4"
	}

	HudHistoryResource	[$WIN32]
	{
		"fieldName" "HudHistoryResource"
		"visible" "1"
		"enabled" "1"
		"xpos"	"r252"
		"ypos"	"40"
		"wide"	 "248"
		"tall"	 "320"

		"history_gap"	"56" [!$OS]
		"history_gap"	"64" [$OSX]
		"icon_inset"	"38"
		"text_inset"	"36"
		"NumberFont"	"HudNumbersSmall"
	}
	HudHistoryResource	[$X360]
	{
		"fieldName" "HudHistoryResource"
		"visible" "1"
		"enabled" "1"
		"xpos"	"r300"
		"ypos"	"40" 
		"wide"	 "248"
		"tall"	 "240"

		"history_gap"	"56"
		"icon_inset"	"38"
		"text_inset"	"36"
		"NumberFont"	"HudNumbersSmall"
	}

	HudGeiger
	{
		"fieldName" "HudGeiger"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HUDQuickInfo
	{
		"fieldName" "HUDQuickInfo"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudWeapon
	{
		"fieldName" "HudWeapon"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}
	HudAnimationInfo
	{
		"fieldName" "HudAnimationInfo"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudPredictionDump
	{
		"fieldName" "HudPredictionDump"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudHintDisplay
	{
		"fieldName"	"HudHintDisplay"
		"visible"	"0"
		"enabled" "1"
		"Alpha"		"0"		// Remove this to enable hint hud element
		"xpos"	"r120"	[$WIN32]
		"ypos"	"r340"	[$WIN32]
		"xpos"	"r148"	[$X360]
		"ypos"	"r338"	[$X360]
		"wide"	"100"
		"tall"	"200"
		"text_xpos"	"8"
		"text_ypos"	"8"
		"text_xgap"	"8"
		"text_ygap"	"8"
		"TextColor"	"255 170 0 220"

		"PaintBackgroundType"	"2"
	}

	HudHintKeyDisplay
	{
		"fieldName"	"HudHintKeyDisplay"
		"visible"	"0"
		"enabled" 	"1"
		"xpos"		"r120"	[$WIN32]
		"ypos"		"r340"	[$WIN32]
		"xpos"		"r148"	[$X360]
		"ypos"		"r338"	[$X360]
		"wide"		"100"
		"tall"		"200"
		"text_xpos"	"8"
		"text_ypos"	"8"
		"text_xgap"	"8"
		"text_ygap"	"8"
		"TextColor"	"255 170 0 220"

		"PaintBackgroundType"	"2"
	}


	HudSquadStatus	[$WIN32]
	{
		"fieldName"	"HudSquadStatus"
		"visible"	"1"
		"enabled" "1"
		"xpos"	"r120"
		"ypos"	"380"
		"wide"	"104"
		"tall"	"46"
		"text_xpos"	"8"
		"text_ypos"	"34"
		"SquadIconColor"	"SquadFg"
		"SquadTextColor"	"SquadFg"
		"IconInsetX"	"8"
		"IconInsetY"	"0"
		"IconGap"		"24"

		"PaintBackgroundType"	"2"
	}
	HudSquadStatus	[$X360]
	{
		"fieldName"	"HudSquadStatus"
		"visible"	"1"
		"enabled" "1"
		"xpos"	"r182"
		"ypos"	"348"
		"wide"	"134"
		"tall"	"62"
		"text_xpos"	"8"
		"text_ypos"	"44"
		"SquadIconColor"	"SquadFg"
		"SquadTextColor"	"SquadFg"
		"IconInsetX"	"8"
		"IconInsetY"	"-4"
		"IconGap"		"24"
		"IconFont"		"SquadIcon"

		"PaintBackgroundType"	"2"
	}

	HudPoisonDamageIndicator	[$WIN32]
	{
		"fieldName"	"HudPoisonDamageIndicator"
		"visible"	"0"
		"enabled" "1"
		"xpos"	"16"
		"ypos"	"346"
		"wide"	"136"
		"tall"	"38"
		"text_xpos"	"8"
		"text_ypos"	"8"
		"text_ygap" "14"
		"TextColor"	"255 170 0 220"
		"PaintBackgroundType"	"2"
	}
	HudPoisonDamageIndicator	[$X360]
	{
		"fieldName"	"HudPoisonDamageIndicator"
		"visible"	"0"
		"enabled" "1"
		"xpos"	"48"
		"ypos"	"264"
		"wide"	"192"
		"tall"	"46"
		"text_xpos"	"8"
		"text_ypos"	"6"
		"text_ygap" "16"
		"TextColor"	"255 170 0 220"
		"PaintBackgroundType"	"2"
	}

	HudCredits
	{
		"fieldName"	"HudCredits"
		"TextFont"	"Default"
		"visible"	"1"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	"640"
		"tall"	"480"
		"TextColor"	"255 255 255 192"

	}
	
	HUDAutoAim
	{
		"fieldName" "HUDAutoAim"
		"visible" "1"
		"enabled" "1"
		"wide"	 "640"	[$WIN32]
		"tall"	 "480"	[$WIN32]
		"wide"	 "960"	[$X360]
		"tall"	 "720"	[$X360]
	}

	HudCommentary
	{
		"fieldName" "HudCommentary"
		"xpos"	"c-190"
		"ypos"	"350"
		"wide"	"380"
		"tall"  "40"
		"visible" "1"
		"enabled" "1"
		
		"PaintBackgroundType"	"2"
		
		"bar_xpos"		"50"
		"bar_ypos"		"20"
		"bar_height"	"8"
		"bar_width"		"320"
		"speaker_xpos"	"50"
		"speaker_ypos"	"8"
		"count_xpos_from_right"	"10"	// Counts from the right side
		"count_ypos"	"8"
		
		"icon_texture"	"vgui/hud/icon_commentary"
		"icon_xpos"		"0"
		"icon_ypos"		"0"		
		"icon_width"	"40"
		"icon_height"	"40"
	}
	
	HudHDRDemo
	{
		"fieldName" "HudHDRDemo"
		"xpos"	"0"
		"ypos"	"0"
		"wide"	"640"
		"tall"  "480"
		"visible" "1"
		"enabled" "1"
		
		"Alpha"	"255"
		"PaintBackgroundType"	"2"
		
		"BorderColor"	"0 0 0 255"
		"BorderLeft"	"16"
		"BorderRight"	"16"
		"BorderTop"		"16"
		"BorderBottom"	"64"
		"BorderCenter"	"0"
		
		"TextColor"		"255 255 255 255"
		"LeftTitleY"	"422"
		"RightTitleY"	"422"
	}
	"HudChatHistory"
	{
		"ControlName"		"RichText"
		"fieldName"		"HudChatHistory"
		"xpos"			"10"
		"ypos"			"290"
		"wide"	 		"300"
		"tall"			 "75"
		"wrap"			"1"
		"autoResize"		"1"
		"pinCorner"		"1"
		"visible"		"0"
		"enabled"		"1"
		"labelText"		""
		"textAlignment"		"south-west"
		"font"			"Default"
		"maxchars"		"-1"
	}

	AchievementNotificationPanel	
	{
		"fieldName"				"AchievementNotificationPanel"
		"visible"				"1"
		"enabled"				"1"
		"xpos"					"0"
		"ypos"					"180"
		"wide"					"f10"	[$WIN32]
		"wide"					"f60"	[$X360]
		"tall"					"100"
	}

	HudDeathNotice
	{
		"fieldName" "HudDeathNotice"
		"visible" "1"
		"enabled" "1"
		"xpos"	 "r640"	[$WIN32]
		"ypos"	 "18"	[$WIN32]
		"xpos"	 "r672"	[$X360]
		"ypos"	 "35"	[$X360]
		"wide"	 "628"
		"tall"	 "468"

		"MaxDeathNotices" "4"
		"IconScale"	  "0.35"
		"LineHeight"	  "16"
		"LineSpacing"	  "4"
		"CornerRadius"	  "3"
		"RightJustify"	  "1"	// If 1, draw notices from the right
		
		"TextFont"		"Default"
		
		"TeamBlue"		"HUDBlueTeamSolid"
		"TeamRed"		"HUDRedTeamSolid"
		"IconColor"		"HudWhite"
		"LocalPlayerColor"	"HUDBlack"

		//"BaseBackgroundColor"	"46 43 42 220"		[$WIN32]
		"BaseBackgroundColor"	"TransparentBlack"
		"LocalBackgroundColor"	"245 229 196 200"	[$WIN32]
		//"BaseBackgroundColor"	"32 32 32 255"		[$X360]
		"LocalBackgroundColor"	"0 0 0 255"		[$X360]
	}
	
	HudPlayerInfo
	{
		"fieldName" "HudPlayerInfo"
		"visible" "0"
		"enabled" "0"
		
		"xpos"	 "20"
		"ypos"	 "20"
		"wide"	 "50"
		"tall"	 "50"
	}

	HudHealthHL1
	{
		"fieldName"		"HudHealthHL1"
		"xpos"	"0"
		"ypos"	"430"
		"wide"	"150"
		"tall"  "50"
		"visible" "0"
		"enabled" "0"
	}
	
	HudSuitHL1
	{
		"fieldName"		"HudSuitHL1"
		"xpos"	"0"
		"ypos"	"430"
		"wide"	"640"
		"tall"  "50"
		"visible" "0"
		"enabled" "0"
	}

	HudAmmoHL1
	{
		"fieldName" "HudAmmoHL1"
		"visible" "0"
		"enabled" "0"
		"xpos"	"r640"
		"tall"	 "480"
		"wide"	"640"
	}

	HudAmmoSecondaryHL1
	{
		"fieldName" "HudAmmoSecondaryHL1"
		"visible" "0"
		"enabled" "0"
		"wide"	 "640"
		"tall"	 "480"
	}
	
	HudFlashlightHL1
	{
		"fieldName" "HudFlashlightHL1"
		"visible" "0"
		"enabled" "0"
		"xpos"	"r640"
		"tall"  "480"
		"wide"	"640"
	}
	
	HudDamageIndicatorHL1
	{
		"fieldName" "HudDamageIndicatorHL1"
		"visible" "0"
		"enabled" "0"
		"wide"	 "640"
		"tall"	 "480"
	}

	HudDamageTilesHL1
	{
		"fieldName" "HudDamageIndicatorHL1"
		"visible" "0"
		"enabled" "0"
		"wide"	 "640"
		"tall"	 "480"
	}
	
	HudTimerLaz
	{
		"fieldName"	"HudTimerLaz"
		"visible"	"1"
		"enabled"	"1"
		"xpos"		"c-51"
		"ypos"		"24"
		"wide"	"102"
		"tall"	"36"
		
		"digit_xpos"	"10"
		"text_ypos"	"26"
		
		"PaintBackgroundType"	"2"
		
		"NumberFont"	"HudTimerFont"
		"NumberGlowFont"	"HudTimerFontGlow"
	}
}