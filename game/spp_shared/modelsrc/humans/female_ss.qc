// Created by Crowbar 0.42.0.0

$ModelName "Humans\female_ss.mdl"

$SurfaceProp "default"

$Contents "solid"

// Only set this if you know what it does, and need it for special circumstances, such as with gibs.
// $IllumPosition -0.904 -4.783 25.094

$MaxEyeDeflection 90

$CDMaterials "models\humans\female_animations\"

$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Include "female_ss_box.qci"

$Include "female_ss_bone.qci"

$Include "female_ss_animation.qci"

$IncludeModel "Humans\bms\female_ss.mdl"
$IncludeModel "Humans\femhl2_ss.mdl"

$IncludeModel "humans\animation\female_holster_anims.mdl"
$IncludeModel "humans\animation\armshield_anims.mdl"