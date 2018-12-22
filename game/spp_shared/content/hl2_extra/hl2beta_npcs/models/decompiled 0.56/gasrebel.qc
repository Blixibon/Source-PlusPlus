// Created by Crowbar 0.56

$ModelName "gasrebel.mdl"

$BodyGroup "Studio"
{
	studio "gasrebel_citizen_17.smd"
}

$Include "gasrebel_lod.qci"

$SurfaceProp "flesh"

$Contents "solid"

$IllumPosition 0.637 0 36.119

$EyePosition 0 0 70

$CDMaterials "models\citizen_17\"

$Attachment "eyes" "ValveBiped.Bip01_Head1" 3.79 -3.78 0.01 rotate 0 -80.1 -90
$Attachment "mouth" "ValveBiped.Bip01_Head1" 1 -5.7 0 rotate 0 -80 -90
$Attachment "chest" "ValveBiped.Bip01_Spine2" 5 4 0 rotate 0 90 90
$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0

// $Include "gasrebel_box.qci"

$Include "gasrebel_bone.qci"

$Include "gasrebel_animation.qci"

$Include "gasrebel_collision.qci"
