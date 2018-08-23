// Created by Crowbar 0.42.0.0

$ModelName "npcs/personality_sphere/personality_sphere_animation.mdl"

$SurfaceProp "default"

$Contents "solid"

// Only set this if you know what it does, and need it for special circumstances, such as with gibs.
// $IllumPosition -0.292 0.082 0.039

$MaxEyeDeflection 90

$MostlyOpaque

$CDMaterials "models\npcs\personality_sphere\.preprocess\"

$Attachment "eyes" "inner_aperture" 6.99 0 0.06 rotate 0 0 0
$Attachment "crush_spark_attach_02" "sphere_root" -4.22 6.07 14.87 rotate 0 0 0
$Attachment "crush_spark_attach_01" "sphere_root" 6.52 -0.97 -14.79 rotate 0 0 0
$Attachment "cable_attach_02" "sphere_root" -14.65 0 -3 rotate 0 0 0
$Attachment "forward" "sphere_root" 17.85 0 0 rotate 0 0 0
$Attachment "spark_attach" "sphere_root" 6.82 -10.42 10.91 rotate 0 0 0
$Attachment "cable_attach_01" "sphere_root" -14.65 0 3 rotate 0 0 0

// $Include "personality_sphere_animation_box.qci"

$Include "personality_sphere_animation_bone.qci"

$Include "personality_sphere_animation_animation.qci"

$KeyValues
{
}
