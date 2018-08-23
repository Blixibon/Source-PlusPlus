// Created by Crowbar 0.42.0.0

$ModelName "cores/puffywheatley.mdl"

$BodyGroup "default"
{
	studio "puffywheatley_reference.smd"
}

$Include "puffywheatley_lod.qci"

$SurfaceProp "default"

$Contents "solid"

// Only set this if you know what it does, and need it for special circumstances, such as with gibs.
// $IllumPosition 74.845 -0.421 59.406

$MaxEyeDeflection 90

$CDMaterials ""

$TextureGroup "skinfamilies"
{
	{ "models\cores\wheatley\personality_sphere_light_damaged" }
	{ "models\cores\wheatley\personality_sphere_light_on"      }
}

// This list shows the VMT file names used in the SMD files.
// "models\cores\wheatley\personality_sphere.vmt"
// "models\cores\wheatley\personality_sphere_glass.vmt"
// "models\cores\wheatley\personality_sphere_glass.vmt"
// "models\cores\wheatley\personality_sphere_light_damaged.vmt"
// "models\cores\wheatley\personality_sphere_light_on.vmt"

$Attachment "eyes" "inner_aperture" 6.99 0 0.06 rotate 0 0 0
$Attachment "crush_spark_attach_01" "sphere_root" 6.52 -0.97 -14.79 rotate 0 0 0
$Attachment "spark_attach" "sphere_root" 6.87 -10.41 12.56 rotate 0 0 0
$Attachment "crush_spark_attach_02" "sphere_root" -4.22 6.07 14.87 rotate 0 0 0
$Attachment "forward" "sphere_root" 17.85 0 0 rotate 0 0 0
$Attachment "cable_attach_01" "sphere_root" -14.65 0 3 rotate 0 0 0
$Attachment "cable_attach_02" "sphere_root" -14.65 0 -3 rotate 0 0 0
$Attachment "upper_handleR" "wing_side_right" 0 0 0 rotate 0 0 0
$Attachment "lower_handleR" "wing_side_right" 0 0 0 rotate 0 0 0
$Attachment "lower_handleL" "wing_side_left" 0 0 0 rotate 0 0 0
$Attachment "upper_handleL" "wing_side_left" 0 0 0 rotate 0 0 0

// $Include "puffywheatley_box.qci"

$Include "puffywheatley_bone.qci"

$Include "puffywheatley_animation.qci"

$Include "puffywheatley_collision.qci"

$KeyValues
{
}
