// Created by Crowbar 0.58

$ModelName "shells\shell_12gauge.mdl"

$BodyGroup "studio"
{
	studio "shell_12gauge_12gauge_reference.smd"
}

$Include "shell_12gauge_lod.qci"

$SurfaceProp "shotgun_shell"

$Contents "solid"

$IllumPosition 0 -0.752 0

$CDMaterials "models\shells\12gauge\"

$Include "shell_12gauge_box.qci"

$Include "shell_12gauge_bone.qci"

$Include "shell_12gauge_animation.qci"

$Include "shell_12gauge_collision.qci"

$keyvalues
{
	prop_data
	{
		base			Metal.Small 
		physicsmode 3
	}
}