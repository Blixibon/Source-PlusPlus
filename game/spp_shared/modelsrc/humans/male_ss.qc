// Created by Crowbar 0.42.0.0

$ModelName "Humans\male_ss.mdl"

$SurfaceProp "default"

$Contents "solid"

// Only set this if you know what it does, and need it for special circumstances, such as with gibs.
// $IllumPosition 0.003 -0.716 33.208

$MaxEyeDeflection 90

$CDMaterials "models\humans\male_animations\"

$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0
$Include "male_ss_box.qci"

$Include "male_ss_bone.qci"

$Include "male_ss_animation.qci"

$IncludeModel "Humans\bms\male_ss.mdl"
$IncludeModel "Humans\mhl2_ss.mdl"

$IncludeModel "humans\animation\male_holster_anims.mdl"
$IncludeModel "humans\animation\armshield_anims.mdl"