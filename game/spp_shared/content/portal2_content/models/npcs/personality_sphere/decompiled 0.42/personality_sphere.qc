// Created by Crowbar 0.42.0.0

$ModelName "npcs/personality_sphere/personality_sphere.mdl"

$BodyGroup "default"
{
	studio "puffywheatley_reference.smd"
}

//$Include "personality_sphere_lod.qci"

$SurfaceProp "default"

$Contents "solid"

// Only set this if you know what it does, and need it for special circumstances, such as with gibs.
// $IllumPosition -0.243 -0.264 0.039

$MaxEyeDeflection 90

$MostlyOpaque

$CDMaterials ""

$TextureGroup "skinfamilies"
{
	{ "models\cores\wheatley\personality_sphere"       "models\cores\wheatley\personality_sphere_light_damaged" }
	{ "models\npcs\personality_sphere\personality_sphere_clean" "models\npcs\personality_sphere\personality_sphere_lights"        }
}

// This list shows the VMT file names used in the SMD files.
// "models\npcs\personality_sphere\personality_sphere.vmt"
// "models\npcs\personality_sphere\personality_sphere_glass.vmt"
// "models\npcs\personality_sphere\personality_sphere_light_damaged.vmt"
// "models\npcs\personality_sphere\personality_sphere_clean.vmt"
// "models\npcs\personality_sphere\personality_sphere_lights.vmt"

$Attachment "eyes" "inner_aperture" 6.99 0 0.06 rotate 0 0 0
$Attachment "crush_spark_attach_01" "sphere_root" 6.52 -0.97 -14.79 rotate 0 0 0
$Attachment "spark_attach" "sphere_root" 6.87 -10.41 12.56 rotate 0 0 0
$Attachment "crush_spark_attach_02" "sphere_root" -4.22 6.07 14.87 rotate 0 0 0
$Attachment "forward" "sphere_root" 17.85 0 0 rotate 0 0 0
$Attachment "cable_attach_01" "sphere_root" -14.65 0 3 rotate 0 0 0
$Attachment "cable_attach_02" "sphere_root" -14.65 0 -3 rotate 0 0 0

$Include "personality_sphere_box.qci"

$Include "puffywheatley_bone.qci"

$Include "personality_sphere_animation.qci"

$Include "personality_sphere_collision.qci"

$KeyValues
{
}
