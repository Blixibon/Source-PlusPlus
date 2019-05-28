// Created by Crowbar 0.58

$ModelName "humans/female_ss.mdl"

$SurfaceProp "default"

$Contents "solid"

$IllumPosition 0.01 -0.091 33.534

$CDMaterials "modelsrc\humans\_animations\female_ss\"

$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0
$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "neck" "ValveBiped.Bip01_Neck1" 3 -4 0 rotate 0 -90 -90

// $Include "female_ss_box.qci"

$Include "female_ss_bone.qci"

$Include "female_ss_animation.qci"
