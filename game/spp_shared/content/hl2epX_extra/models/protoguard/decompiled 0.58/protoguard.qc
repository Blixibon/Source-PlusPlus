// Created by Crowbar 0.58

$ModelName "protoguard\protoguard.mdl"

$BodyGroup "Body"
{
	studio "protoguard_riot_ru2_reference.smd"
}

$Include "protoguard_lod.qci"

$SurfaceProp "flesh"

$Contents "solid"

$IllumPosition 0.02 0.14 37.005

$EyePosition 0 0 73

$CDMaterials "models\nova prospekt\protoguard\"

$Attachment "lefthand" "ValveBiped.Bip01_L_Hand" 0 0 0 rotate 0 0 0
$Attachment "zipline" "ValveBiped.Bip01_Spine4" 3 -4 0 rotate 0 0 0
$Attachment "eyes" "ValveBiped.Bip01_Head1" 4.5 -5 0 rotate 0 -90 -90
$Attachment "beam_damage" "ValveBiped.Bip01_Spine2" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0

$Include "male_04_box.qci"

$Include "protoguard_bone.qci"

$Include "protoguard_animation.qci"

$Include "protoguard_collision.qci"
