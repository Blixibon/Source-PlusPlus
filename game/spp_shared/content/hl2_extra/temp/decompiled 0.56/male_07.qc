// Created by Crowbar 0.56

$ModelName "minic23/citizens/male_07.mdl"

$Model "male_07" "male_07_reference.smd" {

	eyeball "eye_right" "ValveBiped.Bip01_Head1" -1.350981 -3.212738 67.219540 "[unknown_texture]" 1 4 "iris_unused" 0.63
	eyeball "eye_left" "ValveBiped.Bip01_Head1" 1.111219 -3.212730 67.164646 "[unknown_texture]" 1 -4 "iris_unused" 0.63
	eyeball "eye_2" "ValveBiped.Bip01_Head1" -1.350981 -3.212539 67.219044 "eyeball_r" 1 4 "iris_unused" 0.5
	eyeball "eye_3" "ValveBiped.Bip01_Head1" 1.111019 -3.212531 67.165043 "eyeball_l" 1 -4 "iris_unused" 0.5


	mouth 0 "mouth" "ValveBiped.Bip01_Head1" 0 1 0

	$Include "male_07_flex.qci"
}
$BodyGroup "torso"
{
	studio "male_07_male_base_torso_citizen_1.smd"
	studio "male_07_male_base_torso_citizen_2.smd"
	studio "male_07_male_base_torso_citizen_3.smd"
	studio "male_07_male_base_torso_refugee_1.smd"
	studio "male_07_male_base_torso_refugee_2.smd"
	studio "male_07_male_base_torso_rebel_1.smd"
	studio "male_07_male_base_torso_rebel_2.smd"
	studio "male_07_male_base_torso_medic_1.smd"
	studio "male_07_male_base_torso_medic_2.smd"
	studio "male_07_male_base_torso_medic_3.smd"
	studio "male_07_male_base_torso_hostage_1.smd"
	studio "male_07_male_base_torso_hostage_2.smd"
	studio "male_07_male_base_torso_combine_1.smd"
	studio "male_07_male_base_torso_combine_2.smd"
	studio "male_07_male_base_torso_combine_3.smd"
}
$BodyGroup "legs"
{
	studio "male_07_male_base_legs_citizen_1.smd"
	studio "male_07_male_base_legs_citizen_2.smd"
	studio "male_07_male_base_legs_refugee.smd"
	studio "male_07_male_base_legs_rebel_1.smd"
	studio "male_07_male_base_legs_rebel_2.smd"
	studio "male_07_male_base_legs_medic.smd"
	studio "male_07_male_base_legs_hostage.smd"
}
$BodyGroup "hands"
{
	studio "male_07_male_base_hands.smd"
	studio "male_07_male_base_hands_glove_fingerless.smd"
	studio "male_07_male_base_hands_glove.smd"
}
$BodyGroup "beanies"
{
	blank
	studio "male_07_beanie_blue.smd"
	studio "male_07_beanie_green.smd"
}
$BodyGroup "glasses"
{
	blank
	studio "male_07_glasses.smd"
}

$Include "male_07_lod.qci"

$SurfaceProp "flesh"

$Contents "solid"

$IllumPosition -0.152 1.019 33.987

$EyePosition 0 0 70

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

$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0
$Attachment "eyes" "ValveBiped.Bip01_Head1" 3.23 -3.49 0.12 rotate 0 -80.1 -90
$Attachment "mouth" "ValveBiped.Bip01_Head1" 0.6 -5.5 0 rotate 0 -80 -90
$Attachment "chest" "ValveBiped.Bip01_Spine2" 5 4 0 rotate 0 90 90
$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0

// $Include "male_07_box.qci"

$Include "male_07_bone.qci"

$Include "male_07_animation.qci"

$Include "male_07_collision.qci"

$KeyValues
{
}
