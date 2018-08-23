// Created by Crowbar 0.42.0.0

$ModelName "npcs/personality_sphere/personality_sphere_50percent.mdl"

$BodyGroup "default"
{
	studio "personality_sphere_50percent_reference.smd"
}

$Include "personality_sphere_50percent_lod.qci"

$SurfaceProp "default"

$Contents "solid"

// Only set this if you know what it does, and need it for special circumstances, such as with gibs.
// $IllumPosition -0.122 -0.132 0.019

$MaxEyeDeflection 90

$CDMaterials ""

$TextureGroup "skinfamilies"
{
	{ "models\npcs\personality_sphere\personality_sphere"        "models\npcs\personality_sphere\personality_sphere_light_damaged" }
	{ "models\npcs\personality_sphere\personality_sphere_skin02" "models\npcs\personality_sphere\personality_sphere_lights02"      }
}

// This list shows the VMT file names used in the SMD files.
// "models\npcs\personality_sphere\personality_sphere.vmt"
// "models\npcs\personality_sphere\personality_sphere_glass.vmt"
// "models\npcs\personality_sphere\personality_sphere_light_damaged.vmt"
// "models\npcs\personality_sphere\personality_sphere_skin02.vmt"
// "models\npcs\personality_sphere\personality_sphere_lights02.vmt"

$Attachment "eyes" "inner_aperture" 3.49 0 0.03 rotate 0 0 0
$Attachment "spark_attach" "sphere_root" 3.41 -5.21 5.45 rotate 0 0 0
$Attachment "cable_attach_02" "sphere_root" -7.32 0 -1.5 rotate 0 0 0
$Attachment "cable_attach_01" "sphere_root" -7.32 0 1.5 rotate 0 0 0
$Attachment "forward" "sphere_root" 8.93 0 0 rotate 0 0 0

$Include "personality_sphere_50percent_box.qci"

$Include "personality_sphere_50percent_bone.qci"

$Include "personality_sphere_50percent_animation.qci"

$KeyValues
{
}
