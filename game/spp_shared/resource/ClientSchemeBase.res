///////////////////////////////////////////////////////////
// Tracker scheme resource file
//
// sections:
//		Colors			- all the colors used by the scheme
//		BaseSettings	- contains settings for app to use to draw controls
//		Fonts			- list of all the fonts used by app
//		Borders			- description of all the borders
//
///////////////////////////////////////////////////////////
Scheme
{
	//////////////////////// COLORS ///////////////////////////
	// color details
	// this is a list of all the colors used by the scheme
	Colors
	{
		// base colors
		"Orange"			"178 82 22 255"
		"OrangeDim"			"178 82 22 120"
		"LightOrange"			"188 112 0 128"
		"GoalOrange"			"255 133 0"

		"White"				"235 235 235 255"
		"Red"				"192 28 0 140"
		"RedSolid"			"192 28 0 255"
		"Blue"				"0 28 162 140"
		"Yellow"			"251 235 202 255"
		"TransparentYellow"		"251 235 202 140"

		//"Black"				"0 0 0 255"
		//Changed black to a NTSC safe color
		
		"Black"				"46 43 42 255"
		"TransparentBlack"		"0 0 0 196"
		"TransparentLightBlack"		"0 0 0 90"
		"FooterBGBlack"			"52 48 55 255"
		
		"HUDBlueTeam"			"104 124 155 127"
		"HUDRedTeam"			"180 92 77 127"
		"HUDSpectator"			"124 124 124 127"
		"HUDBlueTeamSolid"		"104 124 155 255"
		"HUDRedTeamSolid"		"180 92 77 255"
		"HUDDeathWarning"		"255 0 0 255"
		"HudWhite"			"255 255 255 255"
		"HudOffWhite"			"200 187 161 255"
		
		"Gray"				"178 178 178 255"

		"Blank"				"0 0 0 0"
		"ForTesting"			"255 0 0 32"
		"ForTesting_Magenta"		"255 0 255 255"
		"ForTesting_MagentaDim"		"255 0 255 120"

		"HudPanelForeground"		"123 110 59 184"
		"HudPanelBackground"		"123 110 59 184"
		"HudPanelBorder"		"255 255 255 102"

		"HudProgressBarActive"		"240 207 78 255"
		"HudProgressBarInActive"	"140 120 73 255"
		"HudProgressBarActiveLow"	"240 30 30 255"
		"HudProgressBarInActiveLow"	"240 30 30 99"	

		"HudTimerProgressActive"	"251 235 202 255"
		"HudTimerProgressInActive"	"52 48 45 255"
		"HudTimerProgressWarning"	"240 30 30 255"
		
		"TanDark"			"117 107 94 255"
		"TanLight"			"235 226 202 255"
		"TanDarker"			"46 43 42 255"
		
		// Building HUD Specific
		"LowHealthRed"			"255 0 0 255"
		"ProgressOffWhite"		"251 235 202 255"
		"ProgressBackground"		"250 234 201 51"
		"HealthBgGrey"			"72 71 69 255"
		
		"ProgressOffWhiteTransparent"	"251 235 202 128"
		
		"LabelDark"			"48 43 42 255"
		"LabelTransparent"		"109 96 80 180"
		
		"BuildMenuActive"		"248 231 198 255"
		
		"DisguiseMenuIconRed"		"192 56 63 255"
		"DisguiseMenuIconBlue"		"92 128 166 255"

 		"MatchmakingDialogTitleColor"		"200 184 151 255"
 		"MatchmakingMenuItemBackground"		"46 43 42 255"
 		"MatchmakingMenuItemBackgroundActive"	"150 71 0 255"	
		"MatchmakingMenuItemTitleColor"		"200 184 151 255"
		"MatchmakingMenuItemDescriptionColor"	"200 184 151 255"
		
		"HTMLBackground"					"95 92 101 255"

		"QualityColorNormal"					"178 178 178 255"
		"QualityColorrarity1"					"77 116 85 255"
		"QualityColorrarity2"					"141 131 75 255"
		"QualityColorrarity3"					"207 106 50 255"
		"QualityColorrarity4"					"134 80 172 255"
		"QualityColorVintage"					"71 98 145 255"
		"QualityColorUnique"					"255 215 0 255"
		"QualityColorCommunity"					"112 176 74 255"
		"QualityColorDeveloper"					"165 15 121 255"
		"QualityColorSelfMade"					"112 176 74 255"
		"QualityColorCustomized"				"71 98 145 255"
		"QualityColorStrange"					"205 155 29 255"

		"Chat.TypingText"					"Orange"
		
		"Mod.Main.Title1.Color"	"255 255 255 255"
	}
	
	///////////////////// BASE SETTINGS ////////////////////////
	//
	// default settings for all panels
	// controls use these to determine their settings
	BaseSettings
	{	
		Rosette.DefaultFgColor			"White"
		Rosette.DefaultBgColor			"Blank"
		Rosette.ArmedBgColor			"Blank"
		Rosette.DisabledBgColor			"Blank"
		Rosette.DisabledBorderColor		"Blank"
		Rosette.LineColor				"192 192 192 128"
		Rosette.DrawBorder				"1"
		Rosette.DefaultFont				DefaultSmall
		Rosette.ArmedFont				Default

		Frame.TopBorderImage				"vgui/menu_backgroud_top"
		Frame.BottomBorderImage				"vgui/menu_backgroud_bottom"
		Frame.SmearColor					"0 0 0 225"		[$X360]
		Frame.SmearColor					"0 0 0 180"		[$WIN32]

		"FgColor"			"255 220 0 100"
		"BgColor"			"0 0 0 76"
		
		"BrightFg"		"255 220 0 255"

		"DamagedBg"			"180 0 0 200"
		"DamagedFg"			"180 0 0 230"
		"BrightDamagedFg"		"255 0 0 255"

		// checkboxes and radio buttons
		"BaseText"					"OffWhite"
		"BrightControlText"			"White"
		"CheckBgColor"				"TransparentBlack"
		"CheckButtonBorder1" 			"Border.Dark" 		// the left checkbutton border
		"CheckButtonBorder2"  			"Border.Bright"		// the right checkbutton border
		"CheckButtonCheck"				"White"				// color of the check itself
		
		// weapon selection colors
		"SelectionNumberFg"		"255 220 0 255"
		"SelectionTextFg"		"255 220 0 255"
		"SelectionEmptyBoxBg" 	"0 0 0 80"
		"SelectionBoxBg" 		"0 0 0 80"
		"SelectionSelectedBoxBg" "0 0 0 80"
		
		"ZoomReticleColor"	"255 220 0 255"

		// HL1-style HUD colors
		"Yellowish"			"255 160 0 255"
		"Normal"			"255 208 64 255"
		"Caution"			"255 48 0 255"

		// Top-left corner of the "Half-Life 2" on the main screen
		"Main.Title1.X"				"76"
		"Main.Title1.Y"				"145"
		"Main.Title1.Y_hidef"		"130"
		"Main.Title1.Color"	"255 255 255 0"
		"Mod.Main.Title1.Color"	"255 255 255 255"

		// Top-left corner of secondary title e.g. "DEMO" on the main screen
		"Main.Title2.X"				"76"
		"Main.Title2.Y"				"190"
		"Main.Title2.Y_hidef"		"174"
		"Main.Title2.Color"	"255 255 255 0"
		"Mod.Main.Title2.Color"	"255 255 255 200"

		// Top-left corner of the menu on the main screen
		"Main.Menu.X"			"32"
		"Main.Menu.X_hidef"		"76"
		"Main.Menu.Y"			"340"
		"Main.Menu.Color"		"168 97 64 255"
		"Menu.TextColor"		"0 0 0 255"
		"Menu.BgColor"			"125 125 125 255"

		// Blank space to leave beneath the menu on the main screen
		"Main.BottomBorder"	"32"

		///HERE
				// vgui_controls color specifications
		Border.Bright					"HudPanelBorder"		// the lit side of a control
		Border.Dark						"HudPanelBorder"		// the dark/unlit side of a control
		Border.Selection				"Blank"				// the additional border color for displaying the default/selected button

		Button.TextColor				"TanLight"
		Button.BgColor					"Blank"
		Button.ArmedTextColor			"TanLight"
		Button.ArmedBgColor				"Red"
		Button.DepressedTextColor		"TanLight"
		Button.DepressedBgColor			"Red"

		CheckButton.TextColor			"TanLight"
		CheckButton.SelectedTextColor	"TanLight"
		CheckButton.BgColor				"TransparentBlack"
		CheckButton.Border1  			"Border.Dark" 		// the left checkbutton border
		CheckButton.Border2  			"Border.Bright"		// the right checkbutton border
		CheckButton.Check				"TanLight"				// color of the check itself

		ComboBoxButton.ArrowColor		"TanLight"
		ComboBoxButton.ArmedArrowColor	"TanLight"
		ComboBoxButton.BgColor			"TransparentBlack"
		ComboBoxButton.DisabledBgColor	"Blank"

		Frame.BgColor					"TransparentBlack"
		Frame.OutOfFocusBgColor			"TransparentBlack"
		Frame.FocusTransitionEffectTime	"0.0"	// time it takes for a window to fade in/out on focus/out of focus
		Frame.TransitionEffectTime		"0.0"	// time it takes for a window to fade in/out on open/close
		Frame.AutoSnapRange				"0"
		FrameGrip.Color1				"Blank"
		FrameGrip.Color2				"Blank"
		FrameTitleButton.FgColor		"Blank"
		FrameTitleButton.BgColor		"Blank"
		FrameTitleButton.DisabledFgColor	"Blank"
		FrameTitleButton.DisabledBgColor	"Blank"
		FrameSystemButton.FgColor		"Blank"
		FrameSystemButton.BgColor		"Blank"
		FrameSystemButton.Icon			""
		FrameSystemButton.DisabledIcon	""
		FrameTitleBar.TextColor			"TanLight"
		FrameTitleBar.BgColor			"Blank"
		FrameTitleBar.DisabledTextColor	"TanDark"
		FrameTitleBar.DisabledBgColor	"Blank"
		
		"Panel.FgColor"			"TanLight"
		"Panel.BgColor"			"Blank"

		GraphPanel.FgColor				"TanLight"
		GraphPanel.BgColor				"TransparentBlack"

		Label.TextDullColor				"TanDark"
		Label.TextColor					"TanLight"
		Label.TextBrightColor			"TanLight"
		Label.SelectedTextColor			"TanLight"
		Label.BgColor					"Blank"
		Label.DisabledFgColor1			"Blank"
		Label.DisabledFgColor2			"LightOrange"

		ListPanel.TextColor					"TanLight"
		ListPanel.BgColor					"TransparentBlack"
		ListPanel.SelectedTextColor			"Black"
		ListPanel.SelectedBgColor			"Red"
		ListPanel.SelectedOutOfFocusBgColor	"Red"
		ListPanel.EmptyListInfoTextColor	"TanLight"

		Menu.TextColor					"TanLight"
		Menu.BgColor					"TransparentBlack"
		Menu.ArmedTextColor				"TanLight"
		Menu.ArmedBgColor				"Red"
		Menu.TextInset					"6"

		Chat.TypingText					"HudWhite"

		ProgressBar.FgColor				"TanLight"
		ProgressBar.BgColor				"TransparentBlack"

		PropertySheet.TextColor			"TanLight"
		PropertySheet.SelectedTextColor	"TanLight"
		PropertySheet.TransitionEffectTime	"0.25"	// time to change from one tab to another

		RadioButton.TextColor			"TanLight"
		RadioButton.SelectedTextColor	"TanLight"

		RichText.TextColor				"TanLight"
		RichText.BgColor				"Blank"
		RichText.SelectedTextColor		"TanLight"
		RichText.SelectedBgColor		"Blank"

				ScrollBarButton.FgColor				"TanLight"
		ScrollBarButton.BgColor				"Blank"
		ScrollBarButton.ArmedFgColor		"TanLight"
		ScrollBarButton.ArmedBgColor		"Blank"
		ScrollBarButton.DepressedFgColor	"TanLight"
		ScrollBarButton.DepressedBgColor	"Blank"

		ScrollBarSlider.FgColor				"Blank"		// nob color
		ScrollBarSlider.BgColor				"Blank"		// slider background color

		SectionedListPanel.HeaderTextColor	"TanLight"
		SectionedListPanel.HeaderBgColor	"Blank"
		SectionedListPanel.DividerColor		"Black"
		SectionedListPanel.TextColor		"TanLight"
		SectionedListPanel.BrightTextColor	"TanLight"
		SectionedListPanel.BgColor			"TransparentLightBlack"
		SectionedListPanel.SelectedTextColor			"Black"
		SectionedListPanel.SelectedBgColor				"Red"
		SectionedListPanel.OutOfFocusSelectedTextColor	"Black"
		SectionedListPanel.OutOfFocusSelectedBgColor	"255 255 255 32"

		Slider.NobColor				"108 108 108 255"
		Slider.TextColor			"127 140 127 255"
		Slider.TrackColor			"31 31 31 255"
		Slider.DisabledTextColor1	"117 117 117 255"
		Slider.DisabledTextColor2	"30 30 30 255"

		TextEntry.TextColor			"TanLight"
		TextEntry.BgColor			"TransparentBlack"
		TextEntry.CursorColor		"HudOffWhite"
		TextEntry.DisabledTextColor	"TanLight"
		TextEntry.DisabledBgColor	"Blank"
		TextEntry.SelectedTextColor	"Black"
		TextEntry.SelectedBgColor	"Red"
		TextEntry.OutOfFocusSelectedBgColor	"Red"
		TextEntry.FocusEdgeColor	"TransparentBlack"

		ToggleButton.SelectedTextColor	"TanLight"

		Tooltip.TextColor			"TransparentBlack"
		Tooltip.BgColor				"Red"

		TreeView.BgColor			"TransparentBlack"

		WizardSubPanel.BgColor		"Blank"

		// scheme-specific colors

		"ViewportBG"		"Blank"
		"team0"			"HUDSpectator" // Spectators
		"team1"			"HUDBlueTeam" // CT's
		"team2"			"HUDRedTeam" // T's

		"MapDescriptionText"	"TanLight" // the text used in the map description window
		"CT_Blue"			"HUDBlueTeamSolid"
		"T_Red"				"HUDRedTeamSolid"
		"Hostage_Yellow"	"Panel.FgColor"
		"HudIcon_Green"		"0 160 0 255"
		"HudIcon_Red"		"160 0 0 255"

		// CHudMenu
		"ItemColor"		"255 167 42 200"	// default 255 167 42 255
		"MenuColor"		"233 208 173 255"
		"MenuBoxBg"		"0 0 0 100"

		// Hint message colors
		"HintMessageFg"			"255 255 255 255"
		"HintMessageBg" 		"0 0 0 60"

		"ProgressBarFg"			"HudOffWhite"
	}

	//////////////////////// BITMAP FONT FILES /////////////////////////////
	//
	// Bitmap Fonts are ****VERY*** expensive static memory resources so they are purposely sparse
	BitmapFontFiles
	{
		// UI buttons, custom font, (256x64)
		"Buttons"		"materials/vgui/fonts/buttons_32.vbf"
	}
	
	//////////////////////// FONTS /////////////////////////////
	//
	// describes all the fonts
	Fonts
	{
		// fonts are used in order that they are listed
		// fonts listed later in the order will only be used if they fulfill a range not already filled
		// if a font fails to load then the subsequent fonts will replace

		"DebugFixed"
		{
			"1"
			{
				"name"		"Courier New"
				"tall"		"14"
				"weight"	"400"
				"antialias" "1"
			}
		}

		"DebugFixedSmall"
		{
			"1"
			{
				"name"		"Courier New"
				"tall"		"14"
				"weight"	"400"
				"antialias" "1"
			}
		}

		DebugOverlay 
		{
			"1"
			{
				"name"		"Courier New"
				"tall"		"14"
				"weight"	"400"
				"outline"	"1"
			}
		}

		Default 
		{
			"1"
			{
				"name"		"Verdana"
				"tall"		"9"
				"weight"	"700"
				"antialias" "1"
				"yres"	"1 599"
			}
			"2"
			{
				"name"		"Verdana"
				"tall"		"12"
				"weight"	"700"
				"antialias" "1"
				"yres"	"600 767"
			}
			"3"
			{
				"name"		"Verdana"
				"tall"		"14"
				"weight"	"900"
				"antialias" "1"
				"yres"	"768 1023"
			}
			"4"
			{
				"name"		"Verdana"
				"tall"		"20"
				"weight"	"900"
				"antialias" "1"
				"yres"	"1024 1199"
			}
			"5"
			{
				"name"		"Verdana"
				"tall"		"24"
				"weight"	"900"
				"antialias" "1"
				"yres"	"1200 10000"
				"additive"	"1"
			}
		}

		"DefaultSmall"
		{
			"1"
			{
				"name"		"Verdana"
				"tall"		"12"
				"weight"	"0"
				"range"		"0x0000 0x017F"
				"yres"	"480 599"
			}
			"2"
			{
				"name"		"Verdana"
				"tall"		"13"
				"weight"	"0"
				"range"		"0x0000 0x017F"
				"yres"	"600 767"
			}
			"3"
			{
				"name"		"Verdana"
				"tall"		"14"
				"weight"	"0"
				"range"		"0x0000 0x017F"
				"yres"	"768 1023"
				"antialias"	"1"
			}
			"4"
			{
				"name"		"Verdana"
				"tall"		"20"
				"weight"	"0"
				"range"		"0x0000 0x017F"
				"yres"	"1024 1199"
				"antialias"	"1"
			}
			"5"
			{
				"name"		"Verdana"
				"tall"		"24"
				"weight"	"0"
				"range"		"0x0000 0x017F"
				"yres"	"1200 6000"
				"antialias"	"1"
			}
			"6"
			{
				"name"		"Arial"
				"tall"		"12"
				"range" 		"0x0000 0x00FF"
				"weight"		"0"
			}
		}

		"DefaultVerySmall"
		{
			"1"
			{
				"name"		"Verdana"
				"tall"		"12"
				"weight"	"0"
				"range"		"0x0000 0x017F" //	Basic Latin, Latin-1 Supplement, Latin Extended-A
				"yres"	"480 599"
			}
			"2"
			{
				"name"		"Verdana"
				"tall"		"13"
				"weight"	"0"
				"range"		"0x0000 0x017F" //	Basic Latin, Latin-1 Supplement, Latin Extended-A
				"yres"	"600 767"
			}
			"3"
			{
				"name"		"Verdana"
				"tall"		"14"
				"weight"	"0"
				"range"		"0x0000 0x017F" //	Basic Latin, Latin-1 Supplement, Latin Extended-A
				"yres"	"768 1023"
				"antialias"	"1"
			}
			"4"
			{
				"name"		"Verdana"
				"tall"		"20"
				"weight"	"0"
				"range"		"0x0000 0x017F" //	Basic Latin, Latin-1 Supplement, Latin Extended-A
				"yres"	"1024 1199"
				"antialias"	"1"
			}
			"5"
			{
				"name"		"Verdana"
				"tall"		"24"
				"weight"	"0"
				"range"		"0x0000 0x017F" //	Basic Latin, Latin-1 Supplement, Latin Extended-A
				"yres"	"1200 6000"
				"antialias"	"1"
			}
			"6"
			{
				"name"		"Verdana"
				"tall"		"12"
				"range" 		"0x0000 0x00FF"
				"weight"		"0"
			}
			"7"
			{
				"name"		"Arial"
				"tall"		"11"
				"range" 		"0x0000 0x00FF"
				"weight"		"0"
			}
		}

		WeaponIcons
		{
			"1"
			{
				"name"		"HalfLife2"
				"tall"		"64"
				"tall_hidef"	"58"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}

		WeaponIconsSelected
		{
			"1"
			{
				"name"		"HalfLife2"
				"tall"		"64"
				"tall_hidef"	"58"
				"weight"	"0"
				"antialias" "1"
				"blur"		"5"
				"scanlines"	"2"
				"additive"	"1"
				"custom"	"1"
			}
		}

		WeaponIconsSmall
		{
			"1"
			{
				"name"		"HalfLife2"
				"tall"		"32"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}

		CSWeaponIcons
		{
			"1"
			{
				"name"		"cs"
				"tall"		"64"
				"tall_hidef"	"58"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}

		CSWeaponIconsSelected
		{
			"1"
			{
				"name"		"cs"
				"tall"		"64"
				"tall_hidef"	"58"
				"weight"	"0"
				"antialias" "1"
				"blur"		"5"
				"scanlines"	"2"
				"additive"	"1"
				"custom"	"1"
			}
		}

		CSWeaponIconsSmall
		{
			"1"
			{
				"name"		"cs"
				"tall"		"32"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}

		CSWeaponIconsAmmo
		{
			"1"
			{
				"name"		"csd"
				"tall"		"32"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}

		OBWeaponIcons
		{
			"1"
			{
				"name"		"ObsidianWeaps"
				"tall"		"64"
				"tall_hidef"	"58"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}

		OBWeaponIconsSelected
		{
			"1"
			{
				"name"		"ObsidianWeaps"
				"tall"		"64"
				"tall_hidef"	"58"
				"weight"	"0"
				"antialias" "1"
				"blur"		"5"
				"scanlines"	"2"
				"additive"	"1"
				"custom"	"1"
			}
		}

		OBWeaponIconsSmall
		{
			"1"
			{
				"name"		"ObsidianWeaps"
				"tall"		"32"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}

		NewWeaponIcons
		{
			"1"
			{
				"name"		"RealBeta's Weapon Icons"
				"tall"		"64"
				"tall_hidef"	"58"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}

		NewWeaponIconsSelected
		{
			"1"
			{
				"name"		"RealBeta's Weapon Icons"
				"tall"		"64"
				"tall_hidef"	"58"
				"weight"	"0"
				"antialias" "1"
				"blur"		"5"
				"scanlines"	"2"
				"additive"	"1"
				"custom"	"1"
			}
		}

		NewWeaponIconsSmall
		{
			"1"
			{
				"name"		"RealBeta's Weapon Icons"
				"tall"		"32"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}
		
		HEWeaponIcons
		{
			"1"
			{
				"name"		"HumanError"
				"tall"		"48"
				"tall_hidef"	"43"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}
		HEWeaponIconsSelected
		{
			"1"
			{
				"name"		"HumanError"
				"tall"		"48"
				"tall_hidef"	"43"
				"weight"	"0"
				"antialias" "1"
				"blur"		"5"
				"scanlines"	"2"
				"additive"	"1"
				"custom"	"1"
			}
		}
		HE_HudFont
		{
			"1"
			{
				"name"		"HumanError"
				"tall"		"28"
				"tall_hidef"	"25"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}
		HE_HudFontGlow
		{
			"1"
			{
				"name"		"HumanError"
				"tall"		"28"
				"tall_hidef"	"25"
				"weight"	"0"
				"antialias" 	"1"
				"blur"		"5"
				"scanlines"	"2"
				"additive"	"1"
				"custom"	"1"
			}
		}

		"CloseCaption_Normal"
		{
			"1"
			{
				"name"		"Tahoma"
				"tall"		"26"
				"weight"	"500"
			}
		}
		"CloseCaption_Italic"
		{
			"1"
			{
				"name"		"Tahoma"
				"tall"		"26"
				"weight"	"500"
				"italic"	"1"
			}
		}
		"CloseCaption_Bold"
		{
			"1"
			{
				"name"		"Tahoma"
				"tall"		"26"
				"weight"	"900"
			}
		}
		"CloseCaption_BoldItalic"
		{
			"1"
			{
				"name"		"Tahoma"
				"tall"		"26"
				"weight"	"900"
				"italic"	"1"
			}
		}

		Crosshairs
		{
			"1"
			{
				"name"		"HalfLife2"
				"tall"		"40"
				"weight"	"0"
				"antialias" "0"
				"additive"	"1"
				"custom"	"1"
				"yres"		"1 10000"
			}
		}

		QuickInfo
		{
			"1"
			{
				"name"		"HL2cross"
				"tall"		"32" //28
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
			}
		}

		HudNumbers
		{
			"1"
			{
				"name"		"HalfLife2"
				"tall"		"32"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}

		SquadIcon
		{
			"1"
			{
				"name"		"HalfLife2"
				"tall"		"50"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}

		HudNumbersGlow
		{
			"1"
			{
				"name"		"HalfLife2"
				"tall"		"32"
				"weight"	"0"
				"blur"		"4"
				"scanlines" "2"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}

		HudNumbersSmall
		{
			"1"
			{
				"name"		"HalfLife2"
				"tall"		"16"
				"weight"	"1000"
				"additive"	"1"
				"antialias" "1"
				"custom"	"1"
			}
		}

		HudSelectionNumbers
		{
			"1"
			{
				"name"		"Verdana"
				"tall"		"11"
				"weight"	"700"
				"antialias" "1"
				"additive"	"1"
			}
		}

		HudHintTextLarge
		{
			"1"
			{
				"name"		"Verdana"
				"tall"		"14"
				"weight"	"1000"
				"antialias" "1"
				"additive"	"1"
			}
		}

		HudHintTextSmall
		{
			"1"
			{
				"name"		"Verdana"
				"tall"		"11"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
			}
		}

		HudSelectionText
		{
			"1"
			{
				"name"		"Verdana"
				"tall"		"8"
				"weight"	"700"
				"antialias" "1"
				"yres"	"1 599"
			}
			"2"
			{
				"name"		"Verdana"
				"tall"		"10"
				"weight"	"700"
				"antialias" "1"
				"yres"	"600 767"
			}
			"3"
			{
				"name"		"Verdana"
				"tall"		"12"
				"weight"	"900"
				"antialias" "1"
				"yres"	"768 1023"
			}
			"4"
			{
				"name"		"Verdana"
				"tall"		"16"
				"weight"	"900"
				"antialias" "1"
				"yres"	"1024 1199"
			}
			"5"
			{
				"name"		"Verdana"
				"tall"		"17"
				"weight"	"1000"
				"antialias" "1"
				"yres"	"1200 10000"
			}
		}

		BudgetLabel
		{
			"1"
			{
				"name"		"Courier New"
				"tall"		"14"
				"weight"	"400"
				"outline"	"1"
			}
		}

		

		GameUIButtons
		{
			"1"
			{
				"bitmap"	"1"
				"name"		"Buttons"
				"scalex"	"0.5"
				"scaley"	"0.5"
			}
		}

		
		// this is the symbol font
		"Marlett"
		{
			"1"
			{
				"name"		"Marlett"
				"tall"		"14"
				"weight"	"0"
				"symbol"	"1"
			}
		}

		"Trebuchet24"
		{
			"1"
			{
				"name"		"Trebuchet MS"
				"tall"		"24"
				"weight"	"900"
				"range"		"0x0000 0x007F"	//	Basic Latin
				"antialias" "1"
				"additive"	"1"
			}
		}

		"Trebuchet18"
		{
			"1"
			{
				"name"		"Trebuchet MS"
				"tall"		"18"
				"weight"	"900"
			}
		}

		ClientTitleFont
		{
			"1"
			{
				"name"  "K12HL2" //
				"tall"			"64"
				"tall_hidef"	"72"
				"weight" "0"
				"additive" "0"
				"antialias" "1"
				"custom"	"1"
			}
		}

		CreditsLogo
		{
			"1"
			{
				"name"		"HalfLife2"
				"tall"		"128"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}
		CreditsText
		{
			"1"
			{
				"name"		"Trebuchet MS"
				"tall"		"20"
				"weight"	"900"
				"antialias" "1"
				"additive"	"1"
			}
		}
		CreditsOutroLogos
		{
			"1"
			{
				"name"		"HalfLife2"
				"tall"		"48"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}
		CreditsOutroText
		{
			"1"
			{
				"name"		"Verdana" [!$OSX]
				"name"		"Courier Bold" [$OSX]
				"tall"		"9"
				"weight"	"900"
				"antialias" "1"
			}
		}
		CenterPrintText
		{
			// note that this scales with the screen resolution
			"1"
			{
				"name"		"Trebuchet MS" [!$OSX]
				"name"		"Helvetica" [$OSX]
				"tall"		"18"
				"weight"	"900"
				"antialias" "1"
				"additive"	"1"
			}
		}

		HDRDemoText
		{
			// note that this scales with the screen resolution
			"1"
			{
				"name"		"Trebuchet MS"
				"tall"		"24"
				"weight"	"900"
				"antialias" "1"
				"additive"	"1"
			}
		}

		"AchievementNotification"
		{
			"1"
			{
				"name"		"Trebuchet MS"
				"tall"		"14"
				"weight"	"900"
				"antialias" "1"
			}
		}

		"ChatFont"
		{
			"1"
			{
				"name"		"Verdana"
				"tall"		"12"
				"weight"	"700"
				"yres"	"480 599"
				"dropshadow"	"1"
			}
			"2"
			{
				"name"		"Verdana"
				"tall"		"13"
				"weight"	"700"
				"yres"	"600 767"
				"dropshadow"	"1"
			}
			"3"
			{
				"name"		"Verdana"
				"tall"		"14"
				"weight"	"700"
				"yres"	"768 1023"
				"dropshadow"	"1"
			}
			"4"
			{
				"name"		"Verdana"
				"tall"		"20"
				"weight"	"700"
				"yres"	"1024 1199"
				"dropshadow"	"1"
			}
			"5"
			{
				"name"		"Verdana"
				"tall"		"24"
				"weight"	"700"
				"yres"	"1200 10000"
				"dropshadow"	"1"
			}
		}
				
		"LargeHUDTitle"
		{
			"1"
			{
				"name"		"Trade Gothic Bold"
				"tall"		"20"
				"weight"	"400"
				"antialias" "1"
			}
		}

		"ItemFontNameSmallest"
		{
			"1"
			{
				"name"		"Impact"
				"tall"		"10"
				"weight"	"500"
				"additive"	"0"
				"antialias" "1"
			}
		}
		"ItemFontNameSmall"
		{
			"1"
			{
				"name"		"Impact"
				"tall"		"11"
				"weight"	"500"
				"additive"	"0"
				"antialias" "1"
			}
		}
		"ItemFontNameLarge"
		{
			"1"
			{
				"name"		"Impact"
				"tall"		"15"
				"weight"	"400"
				"antialias"	"1"
			}
		}
		"ItemFontAttribSmallest"
		{
			"1"
			{
				"name"		"Impact"
				"tall"		"7"
				"weight"	"500"
				"additive"	"0"
				"antialias" 	"1"
			}
		}
		"ItemFontAttribSmall"
		{
			"1"
			{
				"name"		"Impact"
				"tall"		"8"
				"weight"	"500"
				"additive"	"0"
				"antialias" 	"1"
			}
		}
		"ItemFontAttribLarge"
		{
			"1"
			{
				"name"		"Impact"
				"tall"		"11"
				"weight"	"500"
				"additive"	"0"
				"antialias" 	"1"
			}
		}

		"CommentaryDefault"
		{
			"1"
			{
				"name"		"Verdana"
				"tall"		"12"
				"weight"	"900"
				"range"		"0x0000 0x017F" //	Basic Latin, Latin-1 Supplement, Latin Extended-A
				"yres"	"480 599"
			}
			"2"
			{
				"name"		"Verdana"
				"tall"		"13"	[$WIN32]
				"tall"		"20"	[$X360]
				"weight"	"900"
				"range"		"0x0000 0x017F" //	Basic Latin, Latin-1 Supplement, Latin Extended-A
				"yres"	"600 767"
			}
			"3"
			{
				"name"		"Verdana"
				"tall"		"14"
				"weight"	"900"
				"range"		"0x0000 0x017F" //	Basic Latin, Latin-1 Supplement, Latin Extended-A
				"yres"	"768 1023"
				"antialias"	"1"
			}
			"4"
			{
				"name"		"Verdana"
				"tall"		"20"
				"weight"	"900"
				"range"		"0x0000 0x017F" //	Basic Latin, Latin-1 Supplement, Latin Extended-A
				"yres"	"1024 1199"
				"antialias"	"1"
			}
			"5"
			{
				"name"		"Verdana"
				"tall"		"24"
				"weight"	"900"
				"range"		"0x0000 0x017F" //	Basic Latin, Latin-1 Supplement, Latin Extended-A
				"yres"	"1200 6000"
				"antialias"	"1"
			}
			"6"
			{
				"name"		"Verdana"
				"tall"		"12"
				"range" 		"0x0000 0x00FF"
				"weight"		"900"
			}
			"7"
			{
				"name"		"Arial"
				"tall"		"12"
				"range" 		"0x0000 0x00FF"
				"weight"		"800"
			}
			
		}
		
		
		Flashlight
		{
			"1"
			{
				"name"		"HalfLife2"
				"tall"		"32"
				"weight"	"0"
				"antialias" "1"
				"additive"	"1"
				"custom"	"1"
			}
		}
	}

	//
	//////////////////// BORDERS //////////////////////////////
	//
	// describes all the border types
	Borders
	{

	}

	//////////////////////// CUSTOM FONT FILES /////////////////////////////
	//
	// specifies all the custom (non-system) font files that need to be loaded to service the above described fonts
	CustomFontFiles 
	{
		"1"		"resource/HALFLIFE2.ttf"
		"2"		"resource/HL2crosshairs.ttf"
		"3"		"resource/HL2EP2.ttf"		
		"4"		"resource/hl2mp.ttf"
		"5"		"resource/cs.ttf"
		"6"		"resource/csd.ttf"
		"7"		"resource/cslogo.ttf"
		"8"		"resource/cstrike.ttf"
		"9"		"resource/RealBeta's Weapon Icons.ttf"
		"10"	"resource/obsidianweaps.ttf"
		"11"	"resource/K12HL2.ttf"
		"12"	"resource/HUMANERROR.ttf"
	}

}
