// Created by Crowbar 0.51.0.0

$modelname "Lenoax\CaveJohnson_NPC.mdl"

$model "Head" "cavejohnson_npc_Head.smd" {

	eyeball "eye_right" "ValveBiped.Bip01_Head1" 1.194800 -2.999983 67.173412 "eyeball_r" 1.05 4 "iris_unused" 0.48
	eyeball "eye_left" "ValveBiped.Bip01_Head1" -1.270500 -2.999983 67.226612 "eyeball_l" 1.05 -4 "iris_unused" 0.48

	eyelid upper_right "cavejohnson_npc.vta" lowerer 20 -0.22 neutral 0 0.03 raiser 21 0.29 split 1 eyeball "eye_right"
	eyelid lower_right "cavejohnson_npc.vta" lowerer 22 -0.37 neutral 0 -0.22 raiser 23 -0.07 split 1 eyeball "eye_right"
	eyelid upper_left "cavejohnson_npc.vta" lowerer 24 -0.2 neutral 0 0.05 raiser 25 0.31 split -1 eyeball "eye_left"
	eyelid lower_left "cavejohnson_npc.vta" lowerer 26 -0.34 neutral 0 -0.2 raiser 27 -0.04 split -1 eyeball "eye_left"

	mouth 0 "mouth" "ValveBiped.Bip01_Head1" 0 1 0

	$include "cavejohnson_npc_flex.qci"
}

$bodygroup "Hair"
{
	studio "cavejohnson_npc_Hair.smd"
}
$bodygroup "Body"
{
	studio "cavejohnson_npc_Body.smd"
}
$bodygroup "Hands"
{
	studio "cavejohnson_npc_Hands.smd"
}

$include "cavejohnson_npc_lod.qci"

$surfaceprop "flesh"

$contents "solid"

// Only set this if you know what it does, and need it for special circumstances, such as with gibs.
// $illumposition 0.003 -0.637 36.052

$maxeyedeflection 90

$forcephonemecrossfade

$mostlyopaque

$cdmaterials "models\Lenoax\CaveJohnson\"
$cdmaterials "models\Gman\"

// This list shows the VMT file names used in the SMD files.
// "upper_teeth.vmt"
// "lower_teeth.vmt"
// "Johnson_Head.vmt"
// "tongue.vmt"
// "eyeball_r.vmt"
// "eyeball_l.vmt"
// "Johnson_Hair.vmt"
// "Johnson_HairTrans.vmt"
// "Johnson_Coat.vmt"
// "Johnson_Body.vmt"
// "v_model_base_arms.vmt"

$attachment "eyes" "ValveBiped.Bip01_Head1" 3.14 -2.77 0 rotate 0 -80.1 -90
$attachment "mouth" "ValveBiped.Bip01_Head1" 0.84 -6.09 -0.16 rotate 0 -80 -90
$attachment "forward" "ValveBiped.Bip01_Head1" 2 -3 0 rotate 0 -76 -90
$attachment "anim_attachment_RH" "ValveBiped.Bip01_R_Hand" 2.68 -1.71 0 rotate 0 0 180
$attachment "anim_attachment_LH" "ValveBiped.Bip01_L_Hand" 2.68 -1.71 0 rotate 0 0 0
$attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0

// $include "cavejohnson_npc_box.qci"

$include "cavejohnson_npc_bone.qci"

$include "cavejohnson_npc_animation.qci"

$include "cavejohnson_npc_collision.qci"
