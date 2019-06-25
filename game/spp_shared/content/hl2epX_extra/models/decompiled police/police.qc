// Created by Crowbar 0.58

$ModelName "player/police.mdl"

$Model "police" "police_reference_hl2.smd" {

	$Include "police_flex.qci"
}
$BodyGroup "manhack"
{
	blank
	studio "police_manhack_reference.smd"
}

$Include "police_lod.qci"

$SurfaceProp "flesh"

$JointSurfaceProp "ValveBiped.Bip01_Head1" "no_decal"
$JointSurfaceProp "ValveBiped.forward" "no_decal"

$Contents "solid"

$IllumPosition -0.026 2.374 35.15

$EyePosition 0 0 72

$CDMaterials "models\Police_hq\"
$CDMaterials "models\manhack\"

$TextureGroup "skinfamilies"
{
	{ "head"      }
	{ "head_glow" }
}

$Attachment "eyes" "ValveBiped.Bip01_Head1" 3.34 -5.28 0 rotate 0 -80.1 -90
$Attachment "Center" "ValveBiped.Bip01_Spine1" -6.2 2.09 0 rotate 0 96.62 90
$Attachment "Chest" "ValveBiped.Bip01_Spine4" -2.7 3.88 0 rotate 0 79.75 90
$Attachment "LHand" "ValveBiped.Bip01_L_Hand" 3 0 0 rotate 0 0 0
$Attachment "RHand" "ValveBiped.Bip01_R_Hand" 3 0 0 rotate 0 0 0
$Attachment "zipline" "ValveBiped.Bip01_Spine4" 2 -2 0 rotate 0 -17 0
$Attachment "manhack" "ValveBiped.Bip01_Spine" 18 -6 3 rotate -25 50 0
$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0

$Include "police_box.qci"

$Include "police_bone.qci"

$Include "police_animation.qci"

$Include "police_collision.qci"

$KeyValues
{
}
