// Created by Crowbar 0.58

$ModelName "shells\shell_9mm.mdl"

$BodyGroup "studio"
{
	studio "shell_9mm_9mm_reference.smd"
}

$Include "shell_9mm_lod.qci"

$SurfaceProp "shell"

$Contents "solid"

$IllumPosition 0 -0.258 0

$CDMaterials "models\shells\9mm\"

$Include "shell_9mm_box.qci"

$Include "shell_9mm_bone.qci"

$Include "shell_9mm_animation.qci"

$Include "shell_9mm_collision.qci"

$keyvalues
{
	prop_data
	{
		base			Metal.Small 
		physicsmode 3
	}
}