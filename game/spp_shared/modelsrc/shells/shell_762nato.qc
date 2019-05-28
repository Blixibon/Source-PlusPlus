// Created by Crowbar 0.58

$ModelName "shells\shell_762nato.mdl"

$BodyGroup "studio"
{
	studio "shell_762nato_762nato_reference.smd"
}

$Include "shell_762nato_lod.qci"

$SurfaceProp "shell"

$Contents "solid"

$IllumPosition 0 -0.998 0

$CDMaterials "models\shells\762nato\"

$Include "shell_762nato_box.qci"

$Include "shell_762nato_bone.qci"

$Include "shell_762nato_animation.qci"

$Include "shell_762nato_collision.qci"

$keyvalues
{
	prop_data
	{
		base			Metal.Small 
		physicsmode 3
	}
}