// Created by Crowbar 0.68

$ModelName "player\HL1\PS2\player.mdl"

$BodyGroup "body"
{
	studio "body_LOD0.smd"
}
$BodyGroup "head"
{
	studio "head_LOD0.smd"
}

$Include "player_lod.qci"

$SurfaceProp "flesh"

$Contents "solid"

$IllumPosition 0.637 0 36.119

$EyePosition 0 0 70

$CDMaterials "models\HL_PS2\player\"
$CDMaterials "models\player\"

$Attachment "eyes" "ValveBiped.Bip01_Head1" 3.79 -3.78 0.01 rotate 0 -80.1 -90
$Attachment "mouth" "ValveBiped.Bip01_Head1" 1 -5.7 0 rotate 0 -80 -90
$Attachment "chest" "ValveBiped.Bip01_Spine2" 5 4 0 rotate 0 90 90
$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0

$Include "player_box.qci"

$Include "player_bone.qci"

$Include "player_animation.qci"

$Include "player_collision.qci"
