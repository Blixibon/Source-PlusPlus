// Created by Crowbar 0.56

$ModelName "humans/gasmask/male_07.mdl"

$Model "male_07" "male_07_reference.smd" {

	eyeball "eye_right" "ValveBiped.Bip01_Head1" -1.350981 -3.212738 67.219540 "eyeball_r" 1 4 "iris_unused" 0.63
	eyeball "eye_left" "ValveBiped.Bip01_Head1" 1.111219 -3.212730 67.164646 "eyeball_l" 1 -4 "iris_unused" 0.63
	//eyeball "eye_2" "ValveBiped.Bip01_Head1" -1.350981 -3.212539 67.219044 "eyeball_r" 1 4 "iris_unused" 0.5
	//eyeball "eye_3" "ValveBiped.Bip01_Head1" 1.111019 -3.212531 67.165043 "eyeball_l" 1 -4 "iris_unused" 0.5


	mouth 0 "mouth" "ValveBiped.Bip01_Head1" 0 1 0

	$Include "male_07_flex.qci"
}

$Include "gasman.qci"

$CDMaterials "models\minic23\citizens\"
$CDMaterials "models\minic23\citizens\facemaps\"

$TextureGroup "skinfamilies"
{
	{ "mike_facemap"  "eyeball_l"       "eyeball_r"       }
	{ "mike_facemap2" "eyeball_l"       "eyeball_r"       }
	{ "mike_facemap3" "eyeball_l"       "eyeball_r"       }
	{ "mike_facemap4" "eyeball_l"       "eyeball_r"       }
	{ "mike_facemap5" "eyeball_l"       "eyeball_r"       }
	{ "mike_facemap6" "eyeball_l"       "eyeball_r"       }
	{ "mike_facemap7" "eyeball_l"       "eyeball_r"       }
	{ "mike_facemap8" "eyeball_l"       "eyeball_r"       }
	{ "mike_facemap"  "eyeball_l_blue"  "eyeball_r_blue"  }
	{ "mike_facemap2" "eyeball_l_blue"  "eyeball_r_blue"  }
	{ "mike_facemap3" "eyeball_l_blue"  "eyeball_r_blue"  }
	{ "mike_facemap4" "eyeball_l_blue"  "eyeball_r_blue"  }
	{ "mike_facemap5" "eyeball_l_blue"  "eyeball_r_blue"  }
	{ "mike_facemap6" "eyeball_l_blue"  "eyeball_r_blue"  }
	{ "mike_facemap7" "eyeball_l_blue"  "eyeball_r_blue"  }
	{ "mike_facemap8" "eyeball_l_blue"  "eyeball_r_blue"  }
	{ "mike_facemap"  "eyeball_l_green" "eyeball_r_green" }
	{ "mike_facemap2" "eyeball_l_green" "eyeball_r_green" }
	{ "mike_facemap3" "eyeball_l_green" "eyeball_r_green" }
	{ "mike_facemap4" "eyeball_l_green" "eyeball_r_green" }
	{ "mike_facemap5" "eyeball_l_green" "eyeball_r_green" }
	{ "mike_facemap6" "eyeball_l_green" "eyeball_r_green" }
	{ "mike_facemap7" "eyeball_l_green" "eyeball_r_green" }
	{ "mike_facemap8" "eyeball_l_green" "eyeball_r_green" }
}


