// Created by Crowbar 0.58

$ModelName "paynamia\bms\gordon_survivor_player.mdl"

$Model "gordon" "gordon_survivor_player_gordon_reference.smd" {

	eyeball "eye_right" "ValveBiped.Bip01_Head1" -1.219999 -3.243221 67.260049 "eyeball_r" 1 4 "iris_unused" 0.56
	eyeball "eye_left" "ValveBiped.Bip01_Head1" 1.230001 -3.253221 67.260049 "eyeball_l" 1 -4 "iris_unused" 0.56

	eyelid upper_right "gordon_survivor_player_01.vta" lowerer 1 -0.15 neutral 0 0.19 raiser 2 0.3 split 1 eyeball "eye_right"
	eyelid lower_right "gordon_survivor_player_01.vta" lowerer 3 -0.31 neutral 0 -0.19 raiser 4 0 split 1 eyeball "eye_right"
	eyelid upper_left "gordon_survivor_player_01.vta" lowerer 6 -0.15 neutral 0 0.19 raiser 7 0.3 split -1 eyeball "eye_left"
	eyelid lower_left "gordon_survivor_player_01.vta" lowerer 8 -0.31 neutral 0 -0.19 raiser 9 0 split -1 eyeball "eye_left"

	mouth 0 "mouth" "ValveBiped.Bip01_Head1" 0 1 0

	$Include "gordon_survivor_player_flex.qci"
}
$BodyGroup "glasses"
{
	studio "gordon_survivor_player_gordon_glasses.smd"
	studio "gordon_survivor_player_gordon_glasses_dirty.smd"
	studio "gordon_survivor_player_bms_gordon_glasses.smd"
	blank
}
$BodyGroup "hev_suit"
{
	studio "gordon_survivor_player_hev_suit.smd"
}
$BodyGroup "arms"
{
	studio "gordon_survivor_player_hev_suit_arms.smd"
	studio "gordon_survivor_player_hev_suit_arms_clean.smd"
}
$BodyGroup "helmet"
{
	blank
	studio "gordon_survivor_player_hev_helmet.smd"
}
$BodyGroup "back_panel"
{
	studio "gordon_survivor_player_hev_suit_panel.smd"
	blank
}

$Include "gordon_survivor_player_lod.qci"

$SurfaceProp "flesh"

$Contents "solid"

$IllumPosition -0.007 0.203 35.329

$EyePosition 0 0 70

$MaxEyeDeflection 65

$CDMaterials "models\paynamia\gordon\"
$CDMaterials "models\bms\humans\hev_suit\"
$CDMaterials "models\paynamia\humans\hev_suit\"
$CDMaterials "models\bms\humans\scientists\"

$TextureGroup "skinfamilies"
{
	{ "gordon_cylmap2_clean" }
	{ "gordon_cylmap2"       }
}

$Attachment "eyes" "ValveBiped.Bip01_Head1" 3.18 -3.52 -0.01 rotate 0 -80.1 -90
$Attachment "mouth" "ValveBiped.Bip01_Head1" 0.8 -5.8 -0.15 rotate 0 -80 -90
$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0
$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0

// $Include "gordon_survivor_player_box.qci"

$Include "gordon_survivor_player_bone.qci"

$Include "gordon_survivor_player_animation.qci"

$Include "gordon_survivor_player_collision.qci"
