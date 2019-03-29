// Created by Crowbar 0.58

$ModelName "Humans\animation\male_holster_anims.mdl"

$SurfaceProp "flesh"

$Contents "solid"

$IllumPosition -0.43 -1.335 35.109

$CDMaterials "models\humans\male_animations\"

$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0
$Attachment "eyes" "ValveBiped.Bip01_Head1" 3.66 -4.21 0 rotate 0 -80.1 -90

$Include "male_shared_box.qci"

$Include "male_shared_bone.qci"

$Include "male_shared_animation.qci"
