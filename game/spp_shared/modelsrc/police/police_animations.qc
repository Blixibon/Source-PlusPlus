// Created by Crowbar 0.58

$ModelName "Police_animations.mdl"

$SurfaceProp "default"

$Contents "solid"

$IllumPosition 0 -0.752 33.456

$CDMaterials "models\police_xsi\"

$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0
$Attachment "eyes" "ValveBiped.Bip01_Head1" 2.7 -7.43 0 rotate 0 -80.1 -90
$Attachment "baton" "ValveBiped.baton_parent" 0 0 0 rotate 0 0 0

$Include "police_animations_box.qci"

$Include "police_animations_bone.qci"

$Include "police_animations_animation.qci"
