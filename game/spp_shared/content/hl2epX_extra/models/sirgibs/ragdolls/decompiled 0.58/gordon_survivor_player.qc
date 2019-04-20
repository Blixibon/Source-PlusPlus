// Created by Crowbar 0.58

$ModelName "SirGibs\Ragdolls\gordon_survivor_player.mdl"
//$ModelName "player.mdl"

$Model "gordon" "gordon_survivor_player_gordon_head_final.smd" {

	eyeball "eye_right" "ValveBiped.Bip01_Head1" -1.260000 -2.530004 67.260021 "eyeball_r" 1 4 "iris_unused" 0.61
	eyeball "eye_left" "ValveBiped.Bip01_Head1" 1.260000 -2.540004 67.260021 "eyeball_l" 1 -4 "iris_unused" 0.61

	eyelid upper_right "gordon_survivor_player_01.vta" lowerer 19 -0.15 neutral 0 0.19 raiser 20 0.3 split 1 eyeball "eye_right"
	eyelid lower_right "gordon_survivor_player_01.vta" lowerer 21 -0.31 neutral 0 -0.19 raiser 22 0 split 1 eyeball "eye_right"
	eyelid upper_left "gordon_survivor_player_01.vta" lowerer 23 -0.15 neutral 0 0.19 raiser 24 0.3 split -1 eyeball "eye_left"
	eyelid lower_left "gordon_survivor_player_01.vta" lowerer 25 -0.31 neutral 0 -0.19 raiser 26 0 split -1 eyeball "eye_left"

	mouth 0 "mouth" "ValveBiped.Bip01_Head1" 0 1 0

	$Include "gordon_survivor_player_flex.qci"
}
$BodyGroup "glasses"
{
	studio "gordon_survivor_player_glasses_new.smd"
}
$BodyGroup "body"
{
	studio "gordon_survivor_player_body_hev.smd"
	studio "gordon_survivor_player_body_hev_hd.smd"
	studio "gordon_survivor_player_hev_suit_bms_new.dmx"
	studio "gordon_survivor_player_body_citizen.smd"
	studio "gordon_survivor_player_body_scientist.smd"
	studio "gordon_body_rebel.dmx"
}

$Include "gordon_survivor_player_lod.qci"

$SurfaceProp "flesh"

$Contents "solid"

$IllumPosition 0.274 -0.142 35.329

$EyePosition 0 0 70

$AmbientBoost

$MostlyOpaque

$CDMaterials "models\SirGibs\gordon_survivor\"
$CDMaterials "models\bms\humans\hev_suit\"
$CDMaterials "models\minic23\citizens\"

$TextureGroup "skinfamilies"
{
	{ "gordon_cylmap2"         "eyeball_l"     "eyeball_r"     "gordon_sheet"     "hevsuit_sheet"     "body"       }
	{ "gordon_cylmap2_ep2"     "eyeball_l"     "eyeball_r"     "gordon_sheet_ep2" "hevsuit_sheet_ep2" "body_dirty" }
	{ "gordon_cylmap2_enh"     "eyeball_l_enh" "eyeball_r_enh" "gordon_sheet"     "hevsuit_sheet"     "body"       }
	{ "gordon_cylmap2_enh_ep2" "eyeball_l_enh" "eyeball_r_enh" "gordon_sheet_ep2" "hevsuit_sheet_ep2" "body_dirty" }
	{ "gordon_cylmap2_enh_bm"  "eyeball_l_enh" "eyeball_r_enh" "gordon_sheet"     "hevsuit_sheet"     "body"       }
}

$Attachment "leftHand" "ValveBiped.Bip01_L_Hand" 3.5 0 0 rotate 0 0 0
$Attachment "RightHand" "ValveBiped.Bip01_R_Hand" 3.5 0 0 rotate 0 0 180
$Attachment "LeftMuzzle" "ValveBiped.Bip01_L_Hand" 12 -1.5 3.5 rotate 0 0 0
$Attachment "RightMuzzle" "ValveBiped.Bip01_R_Hand" 12 -1.5 -3.5 rotate 0 0 180
$Attachment "Eye" "ValveBiped.Bip01_Head1" 3.5 -5 0 rotate 0 -90 0
$Attachment "eyes" "ValveBiped.Bip01_Head1" 3.18 -2.81 -0.01 rotate 0 -80.1 -90
$Attachment "mouth" "ValveBiped.Bip01_Head1" 0.8 -5.8 -0.15 rotate 0 -80 -90
$Attachment "wansyo" "ValveBiped.wansyo" 0 0 0 rotate 0 0 0
$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0

$Include "gordon_survivor_player_box.qci"

$Include "gordon_survivor_player_bone.qci"

$Include "gordon_survivor_player_animation.qci"

$Include "gordon_survivor_player_collision.qci"
